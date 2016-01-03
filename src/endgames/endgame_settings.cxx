#include "endgame_settings.hxx"

EndgameSettings::EndgameSettings(Settings *_settings) : SettingListener(_settings, "Endgame_") {
  set_settings(_settings);
}

void EndgameSettings::set_settings(Settings *_settings) {
  settings = _settings;

  bdd_mate_depth_round_up_to_multiples_of_n =
      get_int("bdd_mate_depth_round_up_to_multiples_of_n");
  do_preprocessing = get_bool("do_preprocessing");
  clustering_method = get_int("clustering_method");
  cluster_functions_preferred = get_bool("cluster_functions_preferred");
  calc_sifting = get_bool("calc_sifting");
  mark_unreachable_as_dont_cares = get_bool("mark_unreachable_as_dont_cares");
  construction_show_progress = get_bool("construction_show_progress");
  do_preprocessing_after_sifting = get_bool("do_preprocessing_after_sifting");

  construction_method = get_int("construction_method");
  unreachability_depth_test = get_int("unreachability_depth_test");
  directory = get_string("directory");

  output_bdd_tables = get_bool("output_bdd_tables");
  output_preprocessed_bdd_tables = get_bool("output_preprocessed_bdd_tables");

  verify_bdd_with_table = get_bool("verify_bdd_with_table");

  reduce_information = get_bool("reduce_information");

  // Square permutations:
  square_enum_white_pawn   = get_int("square_enum_white_pawn");
  square_enum_white_knight = get_int("square_enum_white_knight");
  square_enum_white_bishop = get_int("square_enum_white_bishop");
  square_enum_white_rook   = get_int("square_enum_white_rook");
  square_enum_white_queen  = get_int("square_enum_white_queen");
  square_enum_white_king   = get_int("square_enum_white_king");

  square_enum_black_pawn   = get_int("square_enum_black_pawn");
  square_enum_black_knight = get_int("square_enum_black_knight");
  square_enum_black_bishop = get_int("square_enum_black_bishop");
  square_enum_black_rook   = get_int("square_enum_black_rook");
  square_enum_black_queen  = get_int("square_enum_black_queen");
  square_enum_black_king   = get_int("square_enum_black_king");
}
