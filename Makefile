# Includes
include ./MakefileVar.mk

.PHONY: docker-build

##@ Utility
help: ## Show this help.
	@awk 'BEGIN {FS = ":.*##"; printf "\nUsage:\n  make \033[36m\033[0m\n"} /^[a-zA-Z_-]+:.*?##/ { printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2 } /^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) } ' $(MAKEFILE_LIST)

build-app: ## Build the Go binary of the iqos-xapp
	@go build -v -o build/_output/${XAPP_NAME} ./cmd/${XAPP_NAME}

docker-build: ## Build the iqos-xapp docker image
	docker image build -t ${IQOS_DOCKER_REPO}:${IQOS_VERSION} -f build/Dockerfile .

docker-push: docker-build ## Push image to Docker Hub
	@docker image push ${IQOS_DOCKER_REPO}:${IQOS_VERSION}

helm-install:
	@helm upgrade --install -n riab iqos-xapp ./deploy/helm-chart/iqos-chart

helm-uninstall:
	@helm uninstall -n riab iqos-xapp

clean: ## Remove all the build artifacts
	rm -rf ./build/_output