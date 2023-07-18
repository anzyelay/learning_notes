# Docker on WSL2 without Docker Desktop (Debian / Ubuntu)

[origin Links and references](https://dev.solita.fi/2021/12/21/docker-on-wsl2-without-docker-desktop.html#:~:text=So%2C%20how%20to%20run%20Docker%20on%20WSL2%20under,the%20following%20Install%20pre-required%20packages%20sudo%20apt%20update)

## Start by removing any old Docker related installations

- On Windows: uninstall Docker Desktop
- On WSL2: sudo apt remove docker docker-engine docker.io containerd runc

## Continue on WSL2 with the following

1. Install pre-required packages

    ```sh
    sudo apt update
    sudo apt install --no-install-recommends apt-transport-https ca-certificates curl gnupg2
    ```

1. Configure package repository

    ```sh
    source /etc/os-release
    # curl -fsSL https://download.docker.com/linux/${ID}/gpg | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/docker.gpg
    # echo "deb [arch=amd64] https://download.docker.com/linux/${ID} ${VERSION_CODENAME} stable" | sudo tee /etc/apt/sources.list.d/docker.list
    curl -fsSL http://repo.huaweicloud.com/docker-ce/linux/${ID}/gpg | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/docker.gpg
    echo "deb [arch=amd64] http://repo.huaweicloud.com/docker-ce/linux/ubuntu jammy stable" | sudo tee /etc/apt/sources.list.d/docker.list
    sudo apt update
    ```

1. Install Docker

    ```sh
    sudo apt install docker-ce docker-ce-cli containerd.io
    ```

1. Add user to group

    ```sh
    sudo usermod -aG docker $USER
    ```

1. Configure dockerd

    ```sh
    DOCKER_DIR=/mnt/wsl/shared-docker
    mkdir -pm o=,ug=rwx "$DOCKER_DIR"
    sudo chgrp docker "$DOCKER_DIR"
    sudo mkdir /etc/docker
    sudo cat > /etc/docker/daemon.json <<eof
    {
    "hosts": ["unix:///mnt/wsl/shared-docker/docker.sock"]
    }
    eof
    ```

    - ***Note! Debian/ubuntu will also need the additional configuration to the same file***
      - "iptables": false

## Now you’re ready to launch dockerd and see if it works 

- Run command: `sudo dockerd` - if the command ends with “API listen on /mnt/wsl/shared-docker/docker.sock”, things are working

- You can perform an additional test by opening a new terminal and running

    ```sh
    docker -H unix:///mnt/wsl/shared-docker/docker.sock run --rm hello-world
    ```

## Ok, things are working? Great!

Then it’s time to create a launch script for dockerd. There are two options, manual & automatic

### To always run dockerd automatically

- Add the following to .bashrc or .profile (make sure “DOCKER_DISTRO” matches your distro, you can check it by running `wsl -l -q` in Powershell)

    ```bash
    DOCKER_DISTRO=`lsb_release -is`
    DOCKER_DIR=/mnt/wsl/shared-docker
    DOCKER_SOCK="$DOCKER_DIR/docker.sock"
    export DOCKER_HOST="unix://$DOCKER_SOCK"
    if [ ! -S "$DOCKER_SOCK" ]; then
        mkdir -pm o=,ug=rwx "$DOCKER_DIR"
        sudo chgrp docker "$DOCKER_DIR"
        /mnt/c/Windows/System32/wsl.exe -d $DOCKER_DISTRO sh -c "nohup sudo -b dockerd < /dev/null > $DOCKER_DIR/dockerd.log 2>&1"
    fi
    ```

### To manually run dockerd

- Add the following to your .bashrc or .profile

    ```bash
    DOCKER_SOCK="/mnt/wsl/shared-docker/docker.sock"
    test -S "$DOCKER_SOCK" && export DOCKER_HOST="unix://$DOCKER_SOCK"
    ```

## Want to go passwordless with the launching of dockerd?

All you need to do is

- sudo visudo

- %docker ALL=(ALL) NOPASSWD: /usr/bin/dockerd

## Adding some finishing touches

To wrap things up, you most likely will want to install docker-compose. You can start by checking up the number of the latest stable version from the Docker Compose documentation and doing the following (we’ll be using version 1.29.2 in this example)

```sh
COMPOSE_VERSION=1.29.2
sudo curl -L "https://github.com/docker/compose/releases/download/$COMPOSE_VERSION/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose
```

## Enable / disable BuildKit (optional)

You may end up wanting to enable/disable BuildKit depending on your use cases (basically to end up with the classic output with Docker), and the easiest way for this is to just add the following to your .bashrc or .profile

```bash
export DOCKER_BUILDKIT=0
export BUILDKIT_PROGRESS=plain
```

## Links and references

- [Docker subscription service agreement](https://www.docker.com/legal/docker-subscription-service-agreement/)
- [BuildKit](https://docs.docker.com/build/)
- [Docker Compose](https://docs.docker.com/compose/install/)
