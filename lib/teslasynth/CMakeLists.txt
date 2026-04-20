idf_component_register(
  SRCS
    "config_parser.cpp"
    "config_patch_update.cpp"
  INCLUDE_DIRS "."
  REQUIRES midi synthesizer
)
