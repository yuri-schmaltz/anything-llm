import { fileURLToPath } from "node:url";
import { dirname, resolve, join } from "node:path";

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

export const rootDir = resolve(__dirname, "..");

export const packageDirectories = Object.freeze({
  server: join(rootDir, "server"),
  collector: join(rootDir, "collector"),
  frontend: join(rootDir, "frontend"),
  docker: join(rootDir, "docker"),
});

export const envFileMap = Object.freeze([
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
]);

export const prismaDatabasePath = join(
  packageDirectories.server,
  "storage",
  "anythingllm.db",
);
