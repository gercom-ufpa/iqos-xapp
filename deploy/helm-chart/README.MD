# Local Helm Chart Implementation

Guide for deploying a Helm Chart locally on a Kubernetes cluster.

## Prerequisites

Before you begin, you need to have the following installed on your machine:

- [Helm](https://helm.sh/docs/intro/install/)


## Step-by-Step

1. **Clone the Repository:**
```bash
git clone git@gitlab.com:gercom-ufpa/openran/iqos-xapp.git
```

2. **Navigate to the Chart Directory:**

```bash
cd iqos-xapp/deploy/helm-chart/
```

3. **Install the Chart:**

```bash
helm install -n riab iqos-xapp ./iqos-chart

```

4. **Uninstall the Chart:**

```bash
helm uninstall -n riab iqos-xapp

```