# AnythingLLM Frontend

This package contains the Vite + React client that powers the AnythingLLM user interface. It handles authentication, workspace management, chat, analytics dashboards, and configuration screens.

## Local development

```bash
# From the repository root
yarn dev:frontend
```

The dev server runs on `http://localhost:3000` by default and proxies API requests to the backend defined in `frontend/.env` (see [`docs/CONFIGURATION.md`](../docs/CONFIGURATION.md)). Update `VITE_SERVER_BASE_URL` if the API server runs elsewhere.

## Project layout

- `src/` – Application source organised by feature modules, shared components, hooks, and locales.
- `public/` – Static assets copied into the production build.
- `scripts/postbuild.js` – Generates additional artefacts after `vite build` (e.g., copying locales for the desktop app).
- `tailwind.config.js` / `postcss.config.js` – Styling pipeline configuration.
- `src/locales/` – i18n resources and tooling (`normalizeEn.mjs`, `verifyTranslations.mjs`).

## Available scripts

- `yarn --cwd frontend dev` – run the Vite dev server (also accessible via `yarn dev:frontend`).
- `yarn --cwd frontend build` – create an optimised production bundle.
- `yarn --cwd frontend preview` – preview the production build locally.
- `yarn --cwd frontend lint` – format and lint the React codebase with Prettier.

## Working with translations

1. Edit English strings in `src/locales/en.json`.
2. Run `yarn normalize:translations` from the repository root to regenerate sorted keys and validate other locales.
3. Update locale-specific README files under `locales/` if guidance changes.

## Adding new features

- Use `src/components/common/` for shared building blocks and `src/utils/` for client-side helpers.
- Prefer `react-query` style hooks when interacting with the API; existing patterns live under `src/hooks/`.
- When introducing breaking UI changes, capture screenshots and update documentation to keep parity with the desktop build assets in `images/`.

Refer back to [`docs/ARCHITECTURE.md`](../docs/ARCHITECTURE.md) for details on how the frontend interacts with the server and collector services.
