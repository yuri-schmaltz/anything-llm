#!/usr/bin/env node
import { spawn } from "node:child_process";
import { fileURLToPath } from "node:url";
import { dirname, join, resolve, relative } from "node:path";
import { access, constants, copyFile, mkdir, rm } from "node:fs/promises";
import fs from "node:fs";
import concurrently from "concurrently";

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const rootDir = resolve(__dirname, "..");

const packageDirectories = {
  server: join(rootDir, "server"),
  collector: join(rootDir, "collector"),
  frontend: join(rootDir, "frontend"),
  docker: join(rootDir, "docker"),
};

const envFileMap = [
  {
    example: join(packageDirectories.frontend, ".env.example"),
    target: join(packageDirectories.frontend, ".env"),
    label: "frontend",
  },
  {
    example: join(packageDirectories.server, ".env.example"),
    target: join(packageDirectories.server, ".env.development"),
    label: "server",
  },
  {
    example: join(packageDirectories.collector, ".env.example"),
    target: join(packageDirectories.collector, ".env"),
    label: "collector",
  },
  {
    example: join(packageDirectories.docker, ".env.example"),
    target: join(packageDirectories.docker, ".env"),
    label: "docker",
  },
];

const commands = {
  async help() {
    printHeader("AnythingLLM project helper");
    console.log(`Usage: node scripts/manage.mjs <command> [options]\n`);
    console.log(`Available commands:`);
    console.log(`  setup               Install dependencies, copy .env files, and prime Prisma.`);
    console.log(`  setup:envs          Copy example environment files without overwriting existing ones.`);
    console.log(`  dev:server          Run the API server in development mode.`);
    console.log(`  dev:collector       Run the ingestion service in development mode.`);
    console.log(`  dev:frontend        Run the web client in development mode.`);
    console.log(`  lint                Run lint scripts for server, frontend, and collector.`);
    console.log(`  dev:all             Start server, frontend, and collector concurrently.`);
    console.log(`  prod:server         Start the API server in production mode.`);
    console.log(`  prod:frontend       Build the production frontend.`);
    console.log(`  prisma:generate     Generate the Prisma client.`);
    console.log(`  prisma:migrate      Apply Prisma migrations (use --dev for migrate dev).`);
    console.log(`  prisma:seed         Seed the Prisma database.`);
    console.log(`  prisma:setup        Run generate, migrate, and seed in sequence.`);
    console.log(`  prisma:reset        Reset the local SQLite database then re-run migrations.`);
    console.log(`  verify:translations Verify locale files from the frontend package.`);
    console.log(`  doctor              Run quick environment checks for common setup issues.`);
  },

  async setup(context) {
    printHeader("Setting up AnythingLLM workspace");
    await installWorkspaceDependencies();
    await commands["setup:envs"]();
    if (!context.flags.get("skip-prisma")) {
      await commands["prisma:setup"](context);
    } else {
      console.log("Skipped Prisma setup because --skip-prisma was provided.");
    }
    console.log("\nSetup complete! Run `yarn dev:server`, `yarn dev:collector`, and `yarn dev:frontend` or `yarn dev:all` to start hacking.\n");
  },

  async "setup:envs"() {
    printHeader("Copying example environment files");
    for (const { example, target, label } of envFileMap) {
      await ensureEnvFile(example, target, label);
    }
    console.log("All environment templates copied. Review the new files before booting services.\n");
  },

  async "dev:server"() {
    await runCommand("yarn", ["dev"], { cwd: packageDirectories.server, label: "server" });
  },

  async "dev:collector"() {
    await runCommand("yarn", ["dev"], { cwd: packageDirectories.collector, label: "collector" });
  },

  async "dev:frontend"() {
    await runCommand("yarn", ["dev"], { cwd: packageDirectories.frontend, label: "frontend" });
  },

  async lint() {
    printHeader("Running lint tasks");
    await runCommand("yarn", ["lint"], { cwd: packageDirectories.server, label: "server" });
    await runCommand("yarn", ["lint"], { cwd: packageDirectories.frontend, label: "frontend" });
    await runCommand("yarn", ["lint"], { cwd: packageDirectories.collector, label: "collector" });
    console.log("\nAll lint tasks completed.\n");
  },

  async "dev:all"() {
    printHeader("Starting all development servers");
    await runConcurrently([
      {
        command: "yarn dev",
        name: "server",
        cwd: packageDirectories.server,
      },
      {
        command: "yarn dev",
        name: "collector",
        cwd: packageDirectories.collector,
      },
      {
        command: "yarn dev",
        name: "frontend",
        cwd: packageDirectories.frontend,
      },
    ]);
  },

  async "prod:server"() {
    await runCommand("yarn", ["start"], { cwd: packageDirectories.server, label: "server" });
  },

  async "prod:frontend"() {
    await runCommand("yarn", ["build"], { cwd: packageDirectories.frontend, label: "frontend" });
  },

  async "prisma:generate"() {
    await prismaCommand(["generate"], "generate");
  },

  async "prisma:migrate"(context) {
    const runDev = context.flags.has("dev");
    const nameFlag = context.flags.get("name");
    const args = runDev ? ["migrate", "dev"] : ["migrate", "deploy"];
    if (runDev && nameFlag) {
      args.push("--name", String(nameFlag));
    }
    await prismaCommand(args, runDev ? "migrate dev" : "migrate deploy");
  },

  async "prisma:seed"() {
    await prismaCommand(["db", "seed"], "seed");
  },

  async "prisma:setup"(context) {
    await commands["prisma:generate"](context);
    await commands["prisma:migrate"](context);
    await commands["prisma:seed"](context);
  },

  async "prisma:reset"(context) {
    printHeader("Resetting Prisma SQLite database");
    const sqliteFile = join(packageDirectories.server, "storage", "anythingllm.db");
    const exists = await fileExists(sqliteFile);
    if (exists) {
      await rm(sqliteFile, { force: true });
      console.log(`Removed existing database file at ${relPath(sqliteFile)}`);
    } else {
      console.log("No SQLite database file found to remove. Skipping deletion step.");
    }
    await commands["prisma:setup"](context);
  },

  async "verify:translations"() {
    printHeader("Verifying translation catalogues");
    await runCommand("node", ["verifyTranslations.mjs"], {
      cwd: join(packageDirectories.frontend, "src", "locales"),
      label: "translations",
    });
  },

  async doctor() {
    printHeader("Running environment checks");
    const checks = [
      {
        name: "Node version >= 18",
        action: () => {
          const [major] = process.versions.node.split(".").map(Number);
          return major >= 18;
        },
      },
      {
        name: "Yarn is available",
        action: () => Boolean(which("yarn")),
      },
      {
        name: "Prisma CLI is available",
        action: () => Boolean(which("npx")),
      },
      {
        name: "Required directories exist",
        action: async () => {
          for (const key of ["server", "collector", "frontend"]) {
            await access(packageDirectories[key], constants.F_OK);
          }
          return true;
        },
      },
    ];

    for (const check of checks) {
      try {
        const result = await check.action();
        console.log(`${result ? "✔" : "✖"} ${check.name}`);
      } catch (err) {
        console.log(`✖ ${check.name}`);
        if (err instanceof Error) {
          console.log(`    ${err.message}`);
        }
      }
    }
    console.log("\nDoctor finished. Resolve ✖ checks before continuing.\n");
  },
};

async function installWorkspaceDependencies() {
  printHeader("Installing package dependencies");
  for (const [label, cwd] of Object.entries(packageDirectories)) {
    if (label === "docker") continue;
    const nodeModulesPath = join(cwd, "node_modules");
    if (await fileExists(nodeModulesPath)) {
      console.log(`• ${label}: dependencies already installed.`);
      continue;
    }
    await runCommand("yarn", ["install"], { cwd, label });
  }
}

async function prismaCommand(args, label) {
  printHeader(`Running Prisma ${label}`);
  await runCommand("npx", ["prisma", ...args], {
    cwd: packageDirectories.server,
    label: `prisma ${label}`,
  });
}

async function ensureEnvFile(examplePath, targetPath, label) {
  const targetExists = await fileExists(targetPath);
  if (targetExists) {
    console.log(`• ${label}: ${relPath(targetPath)} already exists. Skipping.`);
    return;
  }

  const exampleExists = await fileExists(examplePath);
  if (!exampleExists) {
    console.warn(`• ${label}: Missing example file at ${relPath(examplePath)}.`);
    return;
  }

  await mkdir(dirname(targetPath), { recursive: true });
  await copyFile(examplePath, targetPath, constants.COPYFILE_EXCL).catch((error) => {
    if (error.code === "EEXIST") return;
    throw error;
  });
  console.log(`• ${label}: created ${relPath(targetPath)} from template.`);
}

async function runCommand(command, args, { cwd = rootDir, label } = {}) {
  const commandLabel = label ? `[${label}]` : command;
  console.log(`→ ${commandLabel} ${args.join(" ")}`.trim());
  return new Promise((resolve, reject) => {
    const child = spawn(command, args, {
      cwd,
      stdio: "inherit",
      shell: process.platform === "win32",
    });

    child.on("close", (code) => {
      if (code === 0) {
        resolve();
      } else {
        reject(new Error(`${commandLabel} exited with code ${code}`));
      }
    });
  });
}

async function runConcurrently(commandList) {
  const { result } = concurrently(commandList, {
    prefix: "name",
    killOthers: ["failure", "success"],
  });
  await result;
}

async function fileExists(path) {
  try {
    await access(path, constants.F_OK);
    return true;
  } catch {
    return false;
  }
}

function relPath(path) {
  return relative(rootDir, path);
}

function printHeader(message) {
  const line = "-".repeat(message.length);
  console.log(`\n${message}\n${line}`);
}

function which(binary) {
  const delimiter = process.platform === "win32" ? ";" : ":";
  const extensions = process.platform === "win32" ? [".cmd", ".exe", ".bat"] : [""];
  const paths = process.env.PATH?.split(delimiter) ?? [];
  for (const base of paths) {
    for (const extension of extensions) {
      const full = join(base, `${binary}${extension}`);
      if (fs.existsSync(full)) return full;
    }
  }
  return null;
}

function parseArgs(argv) {
  const [command, ...rest] = argv;
  const flags = new Map();
  const positional = [];

  for (let i = 0; i < rest.length; i += 1) {
    const current = rest[i];
    if (!current.startsWith("--")) {
      positional.push(current);
      continue;
    }

    const flagBody = current.slice(2);
    const [key, inlineValue] = flagBody.split("=");
    if (inlineValue !== undefined) {
      flags.set(key, inlineValue);
      continue;
    }

    const next = rest[i + 1];
    if (next && !next.startsWith("--")) {
      flags.set(key, next);
      i += 1;
    } else {
      flags.set(key, true);
    }
  }

  return { command, flags, positional };
}

(async () => {
  const { command, flags, positional } = parseArgs(process.argv.slice(2));
  const task = command ? commands[command] : commands.help;

  if (!task) {
    console.error(`Unknown command: ${command}`);
    await commands.help();
    process.exitCode = 1;
    return;
  }

  try {
    await task({ flags, positional });
  } catch (error) {
    if (error instanceof Error) {
      console.error(`\nError: ${error.message}`);
    } else {
      console.error("\nAn unknown error occurred.");
    }
    process.exitCode = 1;
  }
})();
