#ifndef _ENDGAME_SETTINGS_
#define _ENDGAME_SETTINGS_

#include "../settings.hxx"

class EndgameSettings : public SettingListener {
public:
  EndgameSettings(Settings *_settings);

  void set_settings(Settings *_settings);

  int *bdd_mate_depth_round_up_to_multiples_of_n;
  bool *do_preprocessing;
  bool *calc_sifting;
  bool *mark_unreachable_as_dont_cares;
  bool *construction_show_progress;
  bool *do_preprocessing_after_sifting;

  int *clustering_method;
  bool *cluster_functions_preferred;

  int *construction_method;
  int *unreachability_depth_test;

  // directory is the path to where the endgame files are
  // eg. "/users/doktoren/public_html/master_thesis/endgames/"
  char *directory;

  bool *output_bdd_tables;
  bool *output_preprocessed_bdd_tables;

  bool *verify_bdd_with_table;

  bool *reduce_information;

  // Square permutations:
  int *square_enum_white_pawn;
  int *square_enum_white_knight;
  int *square_enum_white_bishop;
  int *square_enum_white_rook;
  int *square_enum_white_queen;
  int *square_enum_white_king;

  int *square_enum_black_pawn;
  int *square_enum_black_knight;
  int *square_enum_black_bishop;
  int *square_enum_black_rook;
  int *square_enum_black_queen;
  int *square_enum_black_king;
};

#endif
