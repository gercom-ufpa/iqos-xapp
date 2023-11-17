FROM golang:1.21.3 as build

ENV XAPPNAME=iqos
WORKDIR /usr/src/github.com/gercom-ufpa/openran/iqos-xapp
COPY ../go.mod ../go.sum ./
RUN go mod download && go mod verify
COPY ../ .
RUN go build -v -o ./build/_output/${XAPPNAME} ./cmd/${XAPPNAME}

FROM alpine:3.18
# TODO: creates appConfig path
RUN apk add libc6-compat
USER nobody
COPY --from=build /usr/src/github.com/gercom-ufpa/openran/iqos-xapp/build/_output/qmai /usr/local/bin/qmai
ENTRYPOINT ["qmai"]