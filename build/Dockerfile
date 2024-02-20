ARG XAPPNAME=iqos-xapp

FROM golang:1.22.0 as build

ARG XAPPNAME
WORKDIR /usr/src/git.rnp.br/openran/fase-1/gts/gt-iqos/${XAPPNAME}
COPY go.mod go.sum ./
RUN go mod download && go mod verify
COPY . .
RUN go build -v -o ./build/_output/${XAPPNAME} ./cmd/${XAPPNAME}/${XAPPNAME}.go

FROM alpine:3.19
# TODO: creates appConfig path
ARG XAPPNAME
RUN apk add libc6-compat
USER nobody
COPY --from=build /usr/src/git.rnp.br/openran/fase-1/gts/gt-iqos/${XAPPNAME}/build/_output/${XAPPNAME} /usr/local/bin/${XAPPNAME}
ENTRYPOINT ["iqos-xapp"]