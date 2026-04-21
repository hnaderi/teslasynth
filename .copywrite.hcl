schema_version = 1

project {
  license        = "GPL-3.0-only"
  copyright_year = 2025
  copyright_holder = "Hossein Naderi"

  # (OPTIONAL) A list of globs that should not have copyright/license headers.
  # Supports doublestar glob patterns for more flexibility in defining which
  # files or folders should be ignored
  header_ignore = [
    "python/**",
    "managed_components/**",
    ".pio/**",
    "build/**",
    "node_modules/**",
    "python/.venv/**",
    "python/build/**",
    "wheelhouse/**",
    "dist/**",
    "web/**/dist/**",
  ]
}
