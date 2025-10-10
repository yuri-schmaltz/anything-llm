#!/usr/bin/env node
import { join } from "node:path";

import { envFileMap, packageDirectories, prismaDatabasePath } from "./config.mjs";
import { ensureFileFromTemplate, fileExists, removeIfExists } from "./lib/fs.mjs";
import { parseArgs } from "./lib/cli.mjs";
import { printHeader, relPath, logCheckResult } from "./lib/output.mjs";
import { runCommand, runConcurrently } from "./lib/process.mjs";
import { which } from "./lib/which.mjs";

const commands = {
  async help() {
    printHeader("AnythingLLM project helper");
    console.log(`Usage: node scripts/manage.mjs <command> [options]\n`);
    console.log(`Available commands:`);
    console.log(`  setup                   Install dependencies, copy .env files, and prime Prisma.`);
    console.log(`  setup:envs              Copy example environment files without overwriting existing ones.`);
    console.log(`  install                 Install dependencies for frontend, server, and collector.`);
    console.log(`  dev:server              Run the API server in development mode.`);
    console.log(`  dev:collector           Run the ingestion service in development mode.`);
    console.log(`  dev:frontend            Run the web client in development mode.`);
    console.log(`  dev:all                 Start server, frontend, and collector concurrently.`);
    console.log(`  lint                    Run lint scripts for server, frontend, and collector.`);
    console.log(`  prod:server             Start the API server in production mode.`);
    console.log(`  prod:frontend           Build the production frontend.`);
    console.log(`  prisma:generate         Generate the Prisma client.`);
    console.log(`  prisma:migrate          Apply Prisma migrations (use --dev for migrate dev).`);
    console.log(`  prisma:seed             Seed the Prisma database.`);
    console.log(`  prisma:setup            Run generate, migrate, and seed in sequence.`);
    console.log(`  prisma:reset            Reset the local SQLite database then re-run migrations.`);
    console.log(`  translations:verify     Verify locale files from the frontend package.`);
    console.log(`  translations:normalize  Sort and normalise translation catalogues, then verify them.`);
    console.log(`  clean:node_modules      Remove per-package node_modules directories.`);
    console.log(`  doctor                  Run quick environment checks for common setup issues.`);
  },

  async setup(context) {
    printHeader("Setting up AnythingLLM workspace");
    const forceInstall = context.flags.has("force-install");
    await installWorkspaceDependencies({ force: forceInstall });
    await commands["setup:envs"]();
    if (!context.flags.get("skip-prisma")) {
      await commands["prisma:setup"](context);
    } else {
      console.log("Skipped Prisma setup because --skip-prisma was provided.");
    }
    console.log("\nSetup complete! Run `yarn dev:server`, `yarn dev:collector`, and `yarn dev:frontend` or `yarn dev:all` to start hacking.\n");
  },

  async install(context) {
    printHeader("Installing package dependencies");
    const forceInstall = context.flags.has("force");
    await installWorkspaceDependencies({ force: forceInstall });
    console.log("\nDependencies installed across packages.\n");
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
    const exists = await fileExists(prismaDatabasePath);
    if (exists) {
      await removeIfExists(prismaDatabasePath);
      console.log(`Removed existing database file at ${relPath(prismaDatabasePath)}`);
    } else {
      console.log("No SQLite database file found to remove. Skipping deletion step.");
    }
    await commands["prisma:setup"](context);
  },

  async "translations:verify"() {
    await runCommand("node", ["verifyTranslations.mjs"], {
      cwd: join(packageDirectories.frontend, "src", "locales"),
      label: "translations",
    });
  },

  async "translations:normalize"(context) {
    printHeader("Normalising translation catalogues");
    const localesDir = join(packageDirectories.frontend, "src", "locales");
    await runCommand("node", ["normalizeEn.mjs"], { cwd: localesDir, label: "normalize" });
    await commands["translations:verify"]();
    if (!context.flags.has("no-lint")) {
      await commands.lint();
    }
    console.log("\nLocales normalised and verified.\n");
  },

  async "clean:node_modules"() {
    printHeader("Removing node_modules directories");
    for (const [label, cwd] of Object.entries(packageDirectories)) {
      if (label === "docker") continue;
      const nodeModulesPath = join(cwd, "node_modules");
      if (!(await fileExists(nodeModulesPath))) {
        console.log(`• ${label}: ${relPath(nodeModulesPath)} already clean.`);
        continue;
      }
      await removeIfExists(nodeModulesPath);
      console.log(`• ${label}: removed ${relPath(nodeModulesPath)}`);
    }
    console.log("\nnode_modules directories removed. Re-run `yarn install` or `yarn install --force` to reinstall dependencies.\n");
  },

  async doctor(context) {
    printHeader("Running environment checks");
    const verbose = context.flags.has("verbose");
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
            await accessDirectory(packageDirectories[key]);
          }
          return true;
        },
      },
      {
        name: ".env files present",
        action: async () => {
          const missing = [];
          for (const { target, label } of envFileMap) {
            if (!(await fileExists(target))) {
              missing.push(`${label} (${relPath(target)})`);
            }
          }
          if (missing.length > 0) {
            if (verbose) {
              console.log(`    Missing env files: ${missing.join(", ")}`);
            }
            return false;
          }
          return true;
        },
      },
    ];

    for (const check of checks) {
      try {
        const result = await check.action();
        logCheckResult(check.name, result);
      } catch (err) {
        const detail = err instanceof Error ? err.message : undefined;
        logCheckResult(check.name, false, detail);
      }
    }
    console.log("\nDoctor finished. Resolve ✖ checks before continuing.\n");
  },
};

async function installWorkspaceDependencies({ force = false } = {}) {
  for (const [label, cwd] of Object.entries(packageDirectories)) {
    if (label === "docker") continue;
    const nodeModulesPath = join(cwd, "node_modules");
    if (!force && (await fileExists(nodeModulesPath))) {
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

  await ensureFileFromTemplate(examplePath, targetPath);
  console.log(`• ${label}: created ${relPath(targetPath)} from template.`);
}

async function accessDirectory(path) {
  if (!(await fileExists(path))) {
    throw new Error(`${relPath(path)} is missing.`);
  }
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
