# AnythingLLM Project Scripts

`scripts/manage.mjs` centralizes the orchestration logic that previously lived inside nested shell commands in `package.json`. The script keeps commands portable across platforms, adds basic error reporting, and exposes a single entry point for frequently used developer tasks.

## Available commands

Run `node ./scripts/manage.mjs help` (or `yarn <command>` thanks to the updated npm scripts) to see all supported commands. The most common tasks are:

- `yarn setup` – install dependencies, copy `.env` templates, and run Prisma generate/migrate/seed.
- `yarn setup:envs` – copy only the `.env` templates without touching existing files.
- `yarn dev:<service>` – boot the `server`, `collector`, or `frontend` in development mode.
- `yarn dev:all` – start all three services with shared logging and automatic teardown on failure.
- `yarn prisma:*` – run Prisma lifecycle steps (`generate`, `migrate`, `seed`, `reset`, `setup`). Add `--dev` to `yarn prisma:migrate` for iterative schema changes.
- `yarn lint` – execute linting across every workspace in a single command.
- `yarn doctor` – perform a quick environment diagnostic (Node version, Yarn availability, Prisma tooling, directory checks).

Because the helper uses Node's `child_process` API with platform-aware fallbacks, commands run consistently on macOS, Linux, and Windows without relying on GNU-specific tools like `cp` or `truncate`.
