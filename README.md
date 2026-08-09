# Sail Prompt

Sail Prompt is a conversational AI chat application for [Sailfish OS](https://sailfishos.org). It talks to
any OpenAI-compatible chat completion API, renders responses as Markdown and lets you export conversations
to Markdown files.

<img src="https://raw.githubusercontent.com/rabauke/harbour-sail-prompt/refs/heads/main/images/screenshot.png" width="480" alt="Sail Prompt screenshot"> 

## Features

- Chat with any OpenAI-compatible chat completion endpoint (base URL, API key and model are configurable).
- Markdown rendering of chat messages using [md4c](https://github.com/mity/md4c).
- Export conversations to Markdown.
- Persistent chat history with the ability to reopen or delete previous sessions.
- Configurable system prompt.
- Translated user interface (English, German, Spanish, French, Swedish).

## Building

Sail Prompt is built with the [Sailfish OS SDK](https://sailfishos.org/wiki/Application_Development) 
using CMake within the Qt Creator of the Sailfish SDK.

The project depends on:

- Qt5 (Core, Network, Qml, Gui, Quick)
- `sailfishapp` (Sailfish OS application framework)
- [md4c](https://github.com/mity/md4c) (bundled in `external/` and built automatically)

## Project Structure

- `src/` — C++ application logic (chat model, OpenAI API client, session storage, Markdown rendering).
- `qml/` — Sailfish Silica UI (pages, cover page, reusable components).
- `rpm/` — RPM packaging spec used to build the Sailfish OS package.
- `translations/` — Qt Linguist translation files.
- `icons/`, `images/` — Application icons and cover image.
- `external/` — Bundled third-party sources (md4c).

## Configuration

Open the settings page in the app to configure:

- **Base URL** — the OpenAI-compatible API endpoint (e.g. `https://api.openai.com/v1`).
- **API Key** — your API key (stored unencrypted in the application settings).
- **System Prompt** — an optional system prompt sent with every conversation.
- **Model** — select from the models available on the configured endpoint.

## Third-Party Components

- [md4c](https://github.com/mity/md4c) — MIT License
- [Qt](https://www.qt.io) — LGPL-2.1-only, GPL-2.0-only
- [Sailfish Silica UI](https://sailfishos.org) — BSD-3-Clause, proprietary
- [Libsailfishapp](https://github.com/sailfishos/libsailfishapp) — LGPL-2.1-only

## License

**Sail Prompt**

Copyright (C) 2026 Heiko Bauke

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

## Author

Copyright © Heiko Bauke, 2026 — [github.com/rabauke/harbour-sail-prompt](https://github.com/rabauke/harbour-sail-prompt)
