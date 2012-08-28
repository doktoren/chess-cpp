#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <stdlib.h>
#include <unistd.h>

#include "xboard_listener.hxx"
#include "engine/cpu_engines.hxx"
#include "file_loader.hxx"
#include "opening_library.hxx"
#include "parser.hxx"
#include "endgames/endgame_database.hxx"
#include "compression/endgame_table_bdd.hxx"
#include "test_suite.hxx"
#include "compression/endgame_piece_enumerations.hxx"

// By making a pointer to cpu, fork() will not duplicate it.
int search_version;
int evaluation_version;
Engine *cpu = 0;

CommandLineReceiver *clr = 0;

FileLoader file_loader;
PGNLoader pgn_loader;

void receive_messages();

int main(int argc, char* argv[]) {
  run_endian_test();
  endgames.init();
  init_opening_library();
  exec_undo_activated = true;

  if (!spawn_listener()) {
    cerr << "Listener exits\n";
    return 0;
  }

  cout << "feature usermove=1\n";
  cout << "feature ping=1\n";
  cout << "feature myname=\"Doctor Chess v0.0\"\n";
  cout << "feature san=1\n";
  cout << "feature analyze=0\n";
  cout << "feature colors=0\n";
  cout << "feature sigint=0\n";
  cout << "feature sigterm=0\n";
  cout << "feature done=1\n";

  endgame_settings = new EndgameSettings(&(comm->settings));

  search_version = *(comm->settings.get_int_setting("Default_search_function", 1));
  evaluation_version = *(comm->settings.get_int_setting("Default_evaluation_function", 1));

  cpu = load_cpu(cpu, search_version, evaluation_version, comm);
  cpu->new_game();

  cerr << "IMPORTANT!!! the command \"load\" must be executed before everything else!\n";

  receive_messages();
  delete cpu;
  delete opening_library;

  im_parant__kill_me();
  //detach_shared_memory();

  cerr << "Engine exits\n";
  return 0;
}

int all_none_gah(int n, int height) {
  if (height == 0) return 1;
  if (height == 1) return n+1;
  return n+1 + n*all_none_gah(n, height-2);
}

int calc_minimal_alpha_beta_tree_size(int n, int height) {
  if (height == 0) return 1;
  if (height == 1) return n+1;
  return calc_minimal_alpha_beta_tree_size(n, height-1) + 1 + (n-1) +
      (n-1)*all_none_gah(n, height-2);
}

void show_minimal_alpha_beta_tree_sizes(ostream &os, int n, int to_depth) {
  os << "Minimal Alpha-Beta tree sizes assuming " << n << " moves from each position:\n";
  for (int i=0; i<=to_depth; i++) {
    os << "h(" << i << ") = " << calc_minimal_alpha_beta_tree_size(n, i);
    if (i != to_depth) os << ", ";
    else os << '\n';
  }
}


// To do: fjern erklæringerne herfra

bool debug_message(string message);

void save_as_pgn(string filename) {
  //cerr << "Saving game as " << filename << "...\n";
  PGNWriter writer(filename);
  writer.output_game(*cpu);
  cerr << "Game saved as " << filename << "\n";
}

void cpu_move() {
  // reply with some move.
  comm->cpu_is_thinking = true;
  Move reply = cpu->calculate_move(cerr);
  comm->cpu_is_thinking = false;
  string s = cpu->moveToSAN(reply);//moveToCAG(reply);
  cpu->execute_move(reply);
  cerr << " - computer replies with move " << s << "\n";
  cout << "move " << s << "\n";

  if (cpu->calc_game_status()) {
    cout << game_status_texts[cpu->game_status_reason] << "\n";
    cerr << game_status_texts[cpu->game_status_reason] << "\n";
  }
}

vector<string> p;

void receive_messages() {
  cerr << "Trying to receive a message...\n";
  // Finite state automata!
  bool load_executed = false;

  char prefix[32];
  prefix[0] = 0;
  char last_command[256];
  last_command[0] = 0;

  do {
    char *message;
    char message_buffer[256];
    vector<string> parsed;

    if ((message = file_loader.getline())  ||  !mq->empty()) {
#ifndef XBOARD
      if (current_cerr) {
        delete current_cerr;
        current_cerr = 0;
        cerr << "Reset cerr!\n";
      }
#endif

      // handle problem with empty lines read from file
      if (message  &&  *message==0) {
        cerr << "message  &&  *message==0\n";
        continue;
      }

      // Prioritize messages from a load command
      if (!message  ||  *message==0) {
        mq->pop(message_buffer);
        message = message_buffer;
        cerr << "Received command: " << message << "\n";
      } else {
        cerr << "Read command from file " << file_loader.getfilename()
	         << ":\n\t\"" << message << "\"\n";
      }

      if (*message == '#') continue;// The message is just a comment

      if (strcmp(message, ",") == 0) {
        cerr << "Last command = " << last_command << '\n';
        strcpy(message, last_command);
      } else {
        strcpy(last_command, message);
      }

      if (strncmp(message, ">>", 2) == 0) {
        char *ch = strchr(message, ':');
        if (ch) {
#ifndef XBOARD
          *ch = 0;
          ofstream *ofs = new ofstream(&(message[2]), ios_base::app);
          ofs->setf(ios::unitbuf);
          if (*ofs) {
            cerr << "Redirection cerr to " << &(message[2]) << '\n';
            current_cerr = ofs;
            message = ch+1;
          } else {
            cerr << "Some error!\n";
            delete ofs;
            continue;
          }
#else
          cerr << "Redirection only supported without XBOARD. Redirection ignored.\n";
          message = ch+1;
#endif
        }
      }

      if (strcmp(message, "leave") == 0) {
        cerr << "leaving current structure.\n";
        clr = 0;
        continue;
      }

      if (message[0]=='.') {
        char tmp[32];
        strcpy(tmp, &(message[1]));
        sprintf(message, "usermove %s", tmp);
        cerr << "Converting to \"" << message << "\"\n";
      }

      cerr << "\nMessage(" << message << ")\n";

      p = parse(message);
      if (p.size() == 0)
        continue;

      if (strcmp(p[0].c_str(), "enter") == 0) {
        if (p.size() < 2) {
          cerr << "enter which structure?\n";
          p = vector<string>(2);
          p[0] = "list";
          p[1] = "structures";

        } else {

          // Erase "enter" from p
          p.erase(p.begin());

          if (dot_demand(p, 1, "board")) {
            cerr << "Commands are now being redirected to (Board)(*cpu)\n";
            clr = cpu->clr_board;

          } else if (strcmp(p[0].c_str(), "board2")==0  ||  strcmp(p[0].c_str(), "b2")==0) {
            cerr << "Commands are now being redirected to (Board2)(*cpu)\n";
            clr = cpu->clr_board2;

          } else if (strcmp(p[0].c_str(), "board2plus")==0  ||  strcmp(p[0].c_str(), "b2p")==0) {
            cerr << "Commands are now being redirected to (Board2plus)(*cpu)\n";
            clr = cpu->clr_board2plus;

          } else if (strcmp(p[0].c_str(), "board3plus")==0  ||  strcmp(p[0].c_str(), "b3p")==0) {
            cerr << "Commands are now being redirected to (Board3plus)(*cpu)\n";
            clr = cpu->clr_board3plus;

          } else if (dot_demand(p, 1, "evaluation")) {
            cerr << "Commands are now being redirected to evaluation part of cpu\n";
            clr = cpu->get_clr_evaluation();

          } else if (dot_demand(p, 1, "search")) {
            cerr << "Commands are now being redirected to search part of cpu\n";
            clr = cpu->get_clr_search();

          } else if (dot_demand(p, 2, "endgame", "database")) {
            cerr << "Commands are now being redirected to endgame database\n";
            clr = clr_endgame_database;

          } else if (dot_demand(p, 2, "test", "suite")) {
            cerr << "Commands are now being redirected to test suite\n";
            clr = TestSuite::clr_test_suite;

          } else {
            cerr << "Unknown structure:";
            for (unsigned int i=0; i<p.size(); i++) {
              cerr << " " << p[i];
            }
            cerr << "\n";
          }

          continue;
        }
      }

      if (clr) {
        if ((*clr)(cpu, cerr, p)) continue;
        cerr << "Current structure did not recognize command\n";
      }

      if (debug_message(message)) {
        // The engine is not connected to xboard.
        // The message is probably to print some debug stuff
        continue;
      } else {
        sscanf(message, "%s", prefix);
        parsed = parse(message);
      }

      if (strcmp(prefix, "loadfen") == 0) {
        char fen[128];
        sscanf(message, "loadfen %[^\"]", fen);
        // cerr << "FEN = \"" << fen << "\"\n";
        cpu->loadFEN(fen);
        cpu->print_board(cerr);

      } else if (strcmp(prefix, "load") == 0) {
        if (strcmp(message, "load") == 0) {
          load_executed = true;
          file_loader.load_file("default.com");
        } else {
          char filename[64];
          sscanf(message, "load %s", filename);
          string name(filename);

          if (name.find(".com") != string::npos)
            file_loader.load_file(name);

          if (name.find(".set") != string::npos)
            comm->settings.load_from_file(name);

          if (name.find(".pgn") != string::npos) {
            pgn_loader.load_file(name);
            if (pgn_loader.next_game()) {

              int max_moves = 999999;
              if (dot_demand(p, 3, "load", (uintptr_t)0, (uintptr_t)0))
                sscanf(p[2].c_str(), "%d", &max_moves);

              pgn_loader.print_tags(cerr);

              pgn_loader.setup_game(*cpu);
              Move move;
              while (cpu->get_moves_played() < max_moves  &&  pgn_loader.next_move(*cpu, move)) {
                // cerr << "Next move is " << move.toString() << "\n";
                cpu->execute_move(move);
                // cpu->print_board(cerr);
              }
              cpu->print_board(cerr, 1);
            } else {
              cerr << "Could not load file\n";
            }
          }
          if (name.find(".fen") != string::npos) {
            cpu->loadFEN(load_FEN(name));
            cpu->print_board(cerr);
          }
        }

      } else if (strcmp(prefix, "save") == 0) {
        if (strcmp(message, "save settings") == 0) {
          comm->settings.save();
        } else {
          char filename[64];
          sscanf(message, "save %s", filename);
          string name(filename);
          if (name.find(".pgn") != string::npos) {
            save_as_pgn(filename);
          }
          if (name.find(".fen") != string::npos) {
            store_FEN(cpu->toFEN(), filename);
          }
        }

      } else if (strcmp(prefix, "cat") == 0) {
        char filename[64];
        sscanf(message, "cat %s", filename);
        char ch;
        ifstream in(filename);
        while (in.get(ch)) cerr << string(1, ch);
        in.close();

      } else if (strcmp(message, "xboard") == 0) {
        cerr << " - ignored\n";
      } else if (strcmp(prefix, "protover") == 0) {
        cerr << " - ignored\n";
      } else if (strcmp(prefix, "accepted") == 0) {
        if (strcmp(message, "accepted done") == 0  &&  !load_executed) {
          cerr << "Loading default_xb.com\n";
          file_loader.load_file("default_xb.com");
        } else {
          cerr << " - ignored\n";
        }

      } else if (strcmp(prefix, "new") == 0) {
        cpu->new_game();
        cerr << " - created new game\n";
        cpu->print_board(cerr);
        comm->cpu_color = BLACK;

      } else if (strcmp(prefix, "random") == 0) {
        cerr << " - random ignored\n";
        // to do:
        // make computer play a bit random


      } else if (strcmp(prefix, "easy") == 0) {
        cerr << " - easy ignored\n";

      } else if (strcmp(prefix, "hard") == 0) {
        cerr << " - hard ignored\n";
        // to do:
        // make computer think in opponents time
      } else if (strcmp(prefix, "ping") == 0) {
        comm->ping_respond = message;
        comm->ping_respond[1] = 'o';
        if (!(comm->cpu_is_thinking)) {
          cout << comm->ping_respond << "\n";
          cerr << " - responded with " << comm->ping_respond << "\n";
        } else {
          cerr << " - delaying response until cpu has moved...\n";
        }
      } else if (strcmp(prefix, "force") == 0) {
        cerr << " - cpu is not playing\n";
        comm->cpu_color = NEITHER_COLOR;

#ifdef XBOARD
      } else if (strcmp(prefix, "time") == 0) {
        sscanf(message, "time %d", &(comm->cpu_time_in_centiseconds));
        cerr << " - cpu has 0.01 * " << comm->cpu_time_in_centiseconds << " seconds left\n";

      } else if (strcmp(prefix, "otim") == 0) {
        cerr << " - otim ignored\n";

      } else if (dot_demand(p, 4, "level", (uintptr_t)0, (uintptr_t)0, (uintptr_t)0)) {
        comm->use_fixed_depth = false;
        comm->fixed_time_per_move = false;
        comm->setting_level_used = true;

        comm->mps = atoi(parse_result[0].c_str());
        if (parse_result[1].find(':') == string::npos) {
          comm->base = 60*atoi(parse_result[1].c_str());
        } else {
          int l = parse_result[1].size();
          comm->base = parse_result[1][--l];
          comm->base += 10*parse_result[1][--l];
          --l;//jump :
          int f = 60;
          do {
            comm->base += f*parse_result[1][--l];
            f *= 10;
          } while (l);
        }
        comm->inc = atoi(parse_result[2].c_str());

        cerr << "Time control: level " << comm->mps << " mps, " << comm->base << " s, "
            << comm->inc << " inc.\n";

      } else if (dot_demand(p, 2, "st", (uintptr_t)0)) {
        comm->use_fixed_depth = false;
        comm->fixed_time_per_move = true;
        comm->setting_level_used = false;

        sscanf(message, "sd %d", &(comm->fixed_depth));
        cerr << " - cpu thinks " << comm->seconds_per_move << " seconds per move\n";

      } else if (strcmp(prefix, "sd") == 0) {
        comm->use_fixed_depth = true;
        comm->fixed_time_per_move = false;
        comm->setting_level_used = false;

        sscanf(message, "sd %d", &(comm->fixed_depth));
        cerr << " - cpu thinking depth set to " << comm->fixed_depth << "\n";

      } else if (dot_demand(p, 1, "computer")) {
        cerr << " - computer ignored\n";
#else
      } else if (strcmp(prefix, "time") == 0  ||
          strcmp(prefix, "otim") == 0  ||
          dot_demand(p, 4, "level", (uintptr_t)0, (uintptr_t)0, (uintptr_t)0)  ||
          dot_demand(p, 2, "st", (uintptr_t)0)  ||
          strcmp(prefix, "sd") == 0  ||
          dot_demand(p, 1, "computer")) {
        cerr << "Program not compiled with XBOARD defined. Command ignored!\n";
#endif

      } else if (strcmp(prefix, "usermove") == 0) {

        {
          int status = cpu->calc_game_status();
          if (status) {
            cout << "ERROR (game is over): " << game_status_texts[cpu->game_status_reason] << "\n";
            continue;
          }
        }

        char move[12];
        sscanf(message, "usermove %s", move);
        cerr << " - move " << move << " played\n";
        int move_accepted = 0;
        Move m = cpu->moves();
        Move the_one;
        while (cpu->next_move(m)) {
          // cout << m.toString() << " as SAN = " << moveToSAN(*cpu, m) << "\n";
          if (cpu->moveToSAN(m) == string(move)) {
            ++move_accepted;
            //cerr << " - the move was recognized!\n";
            the_one = m;
          }
        }
        if (move_accepted == 0) {
          cout << "Illegal move: " << move << "\n";
          continue;
        } else if (move_accepted > 1) {
          cout << "Error (ambiguous move): " << move << '\n';
          continue;
        }

        cpu->execute_move(the_one);

        save_as_pgn("backup.pgn");

        if (cpu->calc_game_status()) {
          cout << game_status_texts[cpu->game_status_reason] << "\n";
          cerr << game_status_texts[cpu->game_status_reason] << "\n";
          continue;
        }

        // cerr << '(' << cpu->player << ',' << cpu->cpu_color << ")\n";
        if (cpu->get_player() == comm->cpu_color) {
          cerr << " - computer is playing current color - an action.\n";
          cpu_move();
        }
        cpu->print_board(cerr, -2);

      } else if (strcmp(prefix, "go") == 0) {
        //cerr << "BEFORE go: " << comm->cpu_color << "\n";
        comm->cpu_color = cpu->get_player();
        //cerr << "AFTER go: " << comm->cpu_color << "\n";
        cpu_move();
        cpu->print_board(cerr, -2);

      } else if (dot_demand(p, 2, "undo", (uintptr_t)0)) {
        comm->cpu_color = NEITHER_COLOR;
        int i = atoi(p[1].c_str());
        while (--i >= 0  &&  cpu->undo_move());
        cpu->print_board(cerr, -2);

      } else if (strcmp(prefix, "undo") == 0) {
        comm->cpu_color = NEITHER_COLOR;
        cpu->undo_move();
        cpu->print_board(cerr, -2);

      } else if (strcmp(prefix, "remove") == 0) {
        cpu->undo_move();
        cpu->undo_move();
        cpu->print_board(cerr, -2);

      } else if (strcmp(prefix, "system") == 0  &&
          strlen(message) >= 8) {
        system(&(message[7]));

      } else if (strcmp(prefix, "quit") == 0) {
        cerr << " - quit\n";
        break;
      } else {
        cerr << "Error (unknown command): " << message << ". Try typing help\n";
      }

    } else {
      // sleep for 40 ms
      usleep(40000);
    }

#ifndef XBOARD
    if (current_cerr) {
      delete current_cerr;
      current_cerr = 0;
      cerr << "Resetted cerr!\n";
    }
#endif

  } while (strcmp(prefix, "quit"));
  cerr << "Engine is terminating...\n";
}

#define print_help(com) cerr << "    " << #com << "  or  " << short_version(#com) << "\n"

bool debug_message(string message) {
  if (dot_demand(p, 1, "help")) {
    cerr << "Overview of commands: (must be typed EXACT)\n"
        << "    help  or  h\n"
        << "    help command  or  hc\n"
        << "      - help on basic commands\n"
        << "    help xboard  or  hx\n"
        << "      - supported XBoard commands\n"
        << "    help file  or  hf\n"
        << "      - file access stuff (fen, pgn, etc)\n"
        << "    enter [structure]\n"
        << "      - Redirect commands to [structure]\n"
        << "    leave\n"
        << "      - leave structure\n"
        << "    list structure  or  ls\n"
        << "      - list current structures.\n"
        << "    >>file: ...\n"
        << "      - execute ... with cerr redirected to file\n"
        << "    system ...\n"
        << "      - executes the system call ... (try eg \"system ls\")\n"
        << "    randseed [n]: set arbitrary randseed or to n\n";

  } else if (dot_demand(p, 2, "help", "command")) {
    cerr << "Commands (those not included in the XBoard command set):\n"
        << "    ,       : Repeat last command (eg. undo/remove)\n"
        << "    dir [n] : Print board. If n is specified, print move list.\n"
        << "      - n<0 : The list is at most -n lines long\n"
        << "      - n=0 : The list has some standard length\n"
        << "      - n>0 : The list starts at move n\n"
        << "    .x      : Make the move x (PGN notation)\n"
        << "    redo [n]: Try to redo a move/n moves, disable cpu.\n"
        << "    pml     : Print list of legal moves\n"
        << "    search version n  or  sv n\n"
        << "      - Change search algorithm to version 1,2 or 3\n"
        << "    evaluation version n  or  ev n\n"
        << "      - Change evaluation function to version 1,2 or 3\n"
        << "    quit    : Noooooo!\n"
        << "    tree size n : Show sizes of minimal alpha-beta trees,\n"
        << "                  assuming exactly n moves from each position.\n";

  } else if (dot_demand(p, 2, "help", "xboard")) {
    cerr << "List of supported XBoard commands:\n"
        << "new         : Creates new game, human white, cpu black.\n"
        << "force       : Disable cpu play.\n"
        << "undo [n]    : Take back last (n) move(s), disable cpu.\n"
        << "remove      : Take back last two move, do not disable cpu.\n"
        << "sd x        : Set cpu thinking depth to x ply.\n"
        << "go          : Set cpu to be current player.\n"
        << "?           : Let cpu play best move found so far (abort search).\n"
        << "usermove x  : Make the move x (PGN notation)\n"
        //<< "setboard FEN: Sets up position according to FEN\n"
        << "hard/easy   : Turn on/off pondering\n";

  } else if (dot_demand(p, 2, "help", "file")) {
    cerr << "File access stuff:\n"
        << "load            : Shortcut for \"load default.com\"\n"
        << "load [file.com] : Execute commands in file.com\n"
        // << "save [file.com] : Save all commands executed (I mean ALL!)\n"
        << "load [file.pgn] : Terminate current game and load file.pgn\n"
        << "save [file.pgn] : Save the current game as file.pgn\n"
        << "load [file.fen] : Terminate current game and load position file.fen\n"
        << "save [file.fen] : Save the current position as file.fen\n"
        << "cat [file]      : Copy file contents to screen\n"
        << "\n"
        << "print settings  or  ps\n"
        << "set name value  or  s name value\n"
        << "      - where name is the name of the setting, and value its new value\n"
        << "load [file.set] : Use file.set as settings file\n"
        << "save settings\n";

#ifdef XBOARD
  } else if (dot_demand(p, 2, "help", "time", "control")) {
    cerr << "Time control:"
        << "level MPS BASE INC : Examples:\n"
        << "    - level 40 5:30 0 : 40 moves per time control,\n"
        << "      5 1/2 minutes per time control, conventional clock\n"
        << "    - level 0 2 12 : Only one clock period,\n"
        << "      base = 2 minutes, inc = 12 second\n"
        << "st TIME  : Allow exactly TIME seconds per move\n"
        << "sd DEPTH : Limit thinking to depth DEPTH\n"
        << "time N : Set cpu's clock to N centiseconds left\n";
#endif

  } else if (dot_demand(p, 2, "list", "structures")) {
    cerr << "List of structures: After eg. \"enter b\", try typing \"help\"\n"
        << "    board (short b)\n"
        << "    board2 (short b2)\n"
        << "    board2plus (short b2p)\n"
        << "    board3plus (short b3p)\n"
        << "    evaluation (short e)\n"
        << "    search (short s)\n"
        << "    endgame database (short ed)\n"
        << "    test suite (short ts)\n";

  } else if (dot_demand(p, 1, "randseed")) {
    int seed = clock() + time(NULL);
    cerr << "Calling srand(" << seed << ")\n";
    srand(seed);

  } else if (dot_demand(p, 2, "randseed", (uintptr_t)0)) {
    int seed = atoi(p[1].c_str());
    cerr << "Calling srand(" << seed << ")\n";
    srand(seed);

  } else if (dot_demand(p, 1, "dir")) {
    cpu->print_board(cerr, -2);

  } else if (dot_demand(p, 2, "dir", (uintptr_t)0)) {
    int from_move = atoi(p[1].c_str());
    cpu->print_board(cerr, from_move);

  } else if (dot_demand(p, 1, "redo")) {
    if (cpu->try_redo_move()) {
      cpu->print_board(cerr, -2);
    } else {
      cerr << "Could not redo move\n";
    }

  } else if (dot_demand(p, 2, "redo", (uintptr_t)0)) {
    int i = atoi(p[1].c_str());
    while (--i >= 0  &&  cpu->try_redo_move()) ;
    cpu->print_board(cerr, -2);

  } else if (dot_demand(p, 1, "pml")) {
    cpu->print_moves(cerr);

  } else if (dot_demand(p, 3, "search", "version", (uintptr_t)1)) {
    int s = p[2][0] - '0';
    if (1<=s  &&  s<=NUM_SEARCH_VERSIONS) {
      search_version = s;
      if (clr) {
        clr = 0;
        cerr << "Leaving structure\n";
      }
      cpu = load_cpu(cpu, search_version, evaluation_version, comm);
    }

  } else if (dot_demand(p, 3, "tree", "size", (uintptr_t)0)) {
    int n = atoi(p[2].c_str());
    show_minimal_alpha_beta_tree_sizes(cerr, n, 12);

  } else if (dot_demand(p, 3, "evaluation", "version", (uintptr_t)1)) {
    int e = p[2][0] - '0';
    if (1<=e  &&  e<=NUM_EVAL_VERSIONS) {
      evaluation_version = e;
      if (clr) {
        clr = 0;
        cerr << "Leaving structure\n";
      }
      cpu = load_cpu(cpu, search_version, evaluation_version, comm);
    }

  } else if (dot_demand(p, 2, "print", "hash_values")) {
    for (int i=1; i<=12; i++) {
      cerr << PPIECE_NAME[i] << ":\n";
      for (int p=0; p<64; p++)
        cerr << "    " << hash_values[(i<<6)|p] << '\n';
    }

  } else if (dot_demand(p, 2, "print", "settings")) {
    comm->settings.print(cerr);

  } else if (dot_demand(p, 3, "set", (uintptr_t)0, (uintptr_t)0)) {
    comm->settings.define(p[1], p[2]);

  } else if (dot_demand(p, 3, "test", "opening", "library")) {
    Move move = cpu->moves();
    while (cpu->next_move(move)) {
      cpu->execute_move(move);
      cerr << move.toString() << " occurs in "
          << opening_library->num_occurences(cpu->hash_value) << " openings.\n";
      cpu->undo_move();
    }

  } else if (dot_demand(p, 3, "index", "opening", "library")) {
    cerr << "todo\n";

  } else {
    return false;
  }

  return true;
}
