# AnythingLLM Collector

The collector is a lightweight Express service dedicated to ingesting and preprocessing raw content before it is embedded by the API server. It supports file uploads, URL scraping, YouTube transcription, and other ingestion flows initiated from the frontend.

## Running locally

```bash
# From the repository root
yarn dev:collector
```

Populate `collector/.env` (via `yarn setup:envs`) to control:

- `COLLECTOR_PORT` – HTTP port exposed to the frontend.
- `TEMP_STORAGE_DIR` – working directory for intermediate artefacts stored in `collector/storage`.
- Provider credentials reused during ingestion (e.g., OpenAI keys for Whisper transcription).

## Directory overview

- `processSingleFile/` – Handlers for uploaded files across supported formats (PDF, DOCX, TXT, XLSX, images, audio, email archives, etc.).
- `processLink/` – Scrapers and parsers for URL-based ingestion.
- `processRawText/` – Utilities for plain text and clipboard submissions.
- `extensions/` – Hooks for browser extensions and other external integrations.
- `utils/` – Shared helpers (chunking, token counting, logging, queue orchestration).
- `hotdir/` – Folder watched for drop-in files when using the desktop build.

## Testing & linting

- Run collector-specific Jest suites:
  ```bash
  yarn --cwd collector test
  ```
- Format files with Prettier:
  ```bash
  yarn --cwd collector lint
  ```

These commands are also triggered by the root aliases (`yarn test`, `yarn lint`).

## Extending ingestion

1. Add a new parser under `processSingleFile/` or `processLink/` depending on the source type.
2. Register the handler inside `index.js` so that the Express router exposes it.
3. If the parser requires additional configuration, document new environment variables in `collector/.env.example` and update [`docs/CONFIGURATION.md`](../docs/CONFIGURATION.md).
4. Cover edge cases with Jest tests in `collector/__tests__/` and run `yarn lint` to keep formatting consistent.

The collector shares Prisma models with the server, so any schema changes should be coordinated via the root Prisma commands.
