# Agent instructions

- Keep changes focused and follow the existing C++17, Qt6 Widgets, FreeType/Fontconfig, CMake, and Nix conventions.
- Use public Qt APIs. Keep the direct FreeType renderer separate from Qt's own font engine; avoid private Qt APIs or global rasterizer settings.
- Keep `VERSION` as the only version source. Follow the development/release rules in `README.md`.
- Run applicable builds and tests when possible; state explicitly what could not be verified.
- Write **detailed commit messages**: an imperative subject and a body explaining the changes, motivation, and verification (including limitations).
- For agent-assisted commits, preserve the **regular user's identity as both Git author and committer**. Credit the chatbot only with a `Co-authored-by: ChatGPT <chatgpt@openai.com>` trailer. Never make the chatbot the author or committer.
- Deliver changes as a single complete Git bundle when requested; new bundles supersede previous ones.
