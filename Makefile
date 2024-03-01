# Includes
include ./MakefileVar.mk

.PHONY: docker-build

##@ Utility
help: ## Show this help.
	@awk 'BEGIN {FS = ":.*##"; printf "\nUsage:\n  make \033[36m\033[0m\n"} /^[a-zA-Z_-]+:.*?##/ { printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2 } /^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) } ' $(MAKEFILE_LIST)

build-app: ## Build the Go binary of the iqos-xapp
	@go build -v -o build/_output/${XAPP_NAME} ./cmd/${XAPP_NAME}

docker-build: ## Build the iqos-xapp docker image
	docker image build -t ${IQOS_DOCKER_REPO}:${IQOS_VERSION} .

docker-push: docker-build ## Push image to Docker Hub
	@docker image push ${IQOS_DOCKER_REPO}:${IQOS_VERSION}

helm-install: ## Install iqos-xapp on Kubernetes
	@helm upgrade --install -n riab iqos-xapp ./deploys/helm-chart/iqos-chart

helm-uninstall: ## Uninstall iqos-xapp on Kubernetes
	@helm uninstall -n riab iqos-xapp

k8s-logs: ## Gets logs from iqos-xapp pod
	@kubectl logs -n riab $$(kubectl get pods -n riab --no-headers -o custom-columns=":metadata.name" | grep iqos-xapp) iqos-xapp -f

clean: ## Remove all the build artifacts
	rm -rf ./build/_output