import { spawn } from "node:child_process";
import concurrently from "concurrently";

export async function runCommand(command, args, { cwd, label } = {}) {
  const commandLabel = label ? `[${label}]` : command;
  const renderedArgs = Array.isArray(args) ? args : [];
  console.log(`→ ${commandLabel} ${renderedArgs.join(" ")}`.trim());

  return new Promise((resolve, reject) => {
    const child = spawn(command, renderedArgs, {
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

export async function runConcurrently(commandList, options = {}) {
  const { result } = concurrently(commandList, {
    prefix: "name",
    killOthers: ["failure", "success"],
    ...options,
  });
  await result;
}
