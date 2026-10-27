# Development workflow

The website is independent of the ESP-IDF firmware. It lives in
`docs/` and uses VitePress with the standard Vue/Vite development
server and static-site build.

## Install site dependencies

From the repository root:

```sh
cd docs
npm install
```

This installs only the website's Node.js dependencies. It does not build,
configure, or flash the firmware.

## Run locally

```sh
npm run docs:dev
```

Open the local URL printed by VitePress (normally `http://localhost:5173`).
The built-in local search indexes the site's documentation.

## Build and preview

```sh
npm run docs:build
npm run docs:preview
```

The build writes static files into `.vitepress/dist/`, ignored by this site's
`.gitignore`. Preview serves that generated output locally. No deployment or
external hosting is performed by these commands.

## Vercel deployment

The repository-root `vercel.json` configures Vercel to install this site's
dependencies from `docs/`, build the VitePress site from `docs/`, and publish
`docs/.vitepress/dist/`. Keep the Vercel project root set to the repository
root so these paths match. Do not override the output directory with
`.vitepress/dist` or set the project root to `docs` while using this
repository-root configuration.

## Firmware commands are separate

Firmware compilation and hardware operation use ESP-IDF commands from the
repository root. See [Build, flash & monitor](/getting-started/build-flash).
Running documentation scripts never invokes `idf.py`.
