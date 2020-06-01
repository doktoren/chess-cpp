run: build
	docker run --rm -it --init --volume $(shell pwd)/tables:/chess/src/tables:rw --volume $(shell pwd)/xboard_commands:/chess/src/xboard_commands:rw --name chess chess

run_db: build_db
	docker run --rm -it --init --volume $(shell pwd)/tables:/chess/src/tables:rw --volume $(shell pwd)/xboard_commands:/chess/src/xboard_commands:rw --name chess_db chess_db

run_xb: build_xb
	docker run --rm -i --init --volume $(shell pwd)/tables:/chess/src/tables:rw --volume $(shell pwd)/xboard_commands:/chess/src/xboard_commands:rw --name chess_xb chess_xb

run_db_xb: build_db_xb
	docker run --rm -i --init --volume $(shell pwd)/tables:/chess/src/tables:rw --volume $(shell pwd)/xboard_commands:/chess/src/xboard_commands:rw --name chess_db_xb chess_db_xb

build_all: build build_db build_xb build_db_xb

build:
	docker build --tag chess .

build_db:
	docker build --build-arg DB=DB --tag chess_db .

build_xb:
	docker build --build-arg XB=XB --tag chess_xb .

build_db_xb:
	docker build --build-arg DB=DB --build-arg XB=XB --tag chess_db_xb .
