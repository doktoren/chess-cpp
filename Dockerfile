FROM gcc:10.1

RUN apt update && apt install -y xboard

WORKDIR /chess/src
ADD src /chess/src

ARG DB=
ARG XB=
RUN DB=$DB XB=$XB make

ENTRYPOINT ["./chess"]
