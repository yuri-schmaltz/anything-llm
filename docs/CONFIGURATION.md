# Configuration Guide

AnythingLLM relies on a small collection of environment files and runtime settings. This guide explains which files exist, what they control, and how to override defaults for different deployment targets.

## Environment templates

Run `yarn setup:envs` to create the following files from their `*.example` counterparts:

| Location | Purpose | Notes |
| --- | --- | --- |
| `frontend/.env` | Controls the Vite dev server and client-side feature flags. | Includes `VITE_SERVER_BASE_URL`, UI toggles, and analytics switches. |
| `server/.env.development` | Configures the Express API (ports, database, auth secrets) plus LLM/vector provider credentials. | Required for local development and many CLI utilities. Copy to `.env` for production or override via hosting platform. |
| `collector/.env` | Defines ingestion server port, concurrency, and temp storage behaviour. | Shares credentials with the API server when embedding jobs require them. |
| `docker/.env` | Contains deployment defaults for the Docker compose stack. | Mirrors many values from the server/collector files for containerised environments. |

> **Tip:** `yarn setup` automatically generates these files if they do not exist. Existing files are never overwritten so that local secrets remain intact.

## Core settings

### Server (`server/.env.development`)

- **Application secrets:** `JWT_SECRET`, `SIG_KEY`, and `SIG_SALT` must be unique per installation. Generate strong random strings before allowing real users to sign in.
- **Database:** `DATABASE_URL` follows Prisma's standard connection string syntax. It defaults to the SQLite file at `server/storage/anythingllm.db` but can target Postgres, MySQL, MSSQL, etc.
- **LLM & embedder selection:** Set `LLM_PROVIDER` and matching credentials (`OPEN_AI_KEY`, `ANTHROPIC_API_KEY`, `OLLAMA_*`, etc.). Each provider block in `.env.example` documents the relevant variables.
- **Vector databases:** `VECTOR_DB`, along with provider-specific URLs/keys (`PINECONE_ENVIRONMENT`, `QDRANT_API_KEY`, ...), enables alternative storage back-ends. Supplemental notes live beside each provider in `server/utils/vectorDbProviders/*/`.
- **Speech services:** Toggle transcription/voice synthesis integrations with variables such as `ENABLE_TTS`, `ELEVENLABS_API_KEY`, etc.

### Collector (`collector/.env`)

- `COLLECTOR_PORT` exposes the ingestion service. Align it with `COLLECTOR_BASE_PATH` in the frontend `.env` when running locally.
- `MAX_CONCURRENT_JOBS` and `MAX_FILE_SIZE` provide coarse-grained control over ingestion throughput.
- Storage paths for temporary files can be customised with `FILE_SYSTEM_ROOT` if using network shares or mounted volumes.

### Frontend (`frontend/.env`)

- `VITE_SERVER_BASE_URL` must point to the server's URL (defaults to `http://localhost:3001`).
- Feature flags such as `VITE_ENABLE_ANALYTICS` and `VITE_ENABLE_OPENAI_STREAMING` allow tailoring the UI during development without rebuilding the server.

## Secrets management

For production deployments avoid committing filled `.env` files. Recommended approaches:

- **Docker Compose:** keep secrets inside `docker/.env` or inject them via your orchestration platform. The compose files load variables from that path automatically.
- **Cloud deployments:** AWS/GCP templates in `cloud-deployments/` expose parameters for credentials and database URLs. Regenerate templates with the root scripts if you need to bake in defaults.
- **Desktop builds:** the Qt installer (`extras/qt-installer/`) expects a populated `server/.env.production` file during packaging. Start from `.env.example` and populate hosted service credentials.

## Prisma database management

Use the helper scripts to manage schema changes:

- `yarn prisma:migrate` – applies pending migrations (`prisma migrate deploy`). Add `--dev --name <migration>` to create new migrations locally.
- `yarn prisma:reset` – removes the local SQLite database and re-runs `prisma:setup` (generate, migrate, seed).
- `yarn prisma:seed` – seeds reference data using `server/prisma/seed.js`.

Migrations live in `server/prisma/migrations/`. Review generated SQL before deploying to shared environments.

## Advanced configuration

- **Overriding per environment:** Most hosting platforms allow injecting environment variables at runtime. Any variable from the `.env` templates can be supplied via the platform instead of editing files.
- **Extending providers:** New LLM or vector integrations belong under `server/utils`. Mirror the structure of existing providers and expose new `.env` variables with clear names to maintain consistency.
- **Feature toggles:** UI-only experiments can use the `VITE_` namespace. Server-side toggles should live in `.env` to keep them off the public bundle.

Consult the root [`README.md`](../README.md) and language-specific docs in `locales/` for exhaustive variable descriptions and provider-specific walkthroughs.
