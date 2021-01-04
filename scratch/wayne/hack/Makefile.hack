# Convenience targets for using the hack demo in the Docker container.

build:
	@docker build -t ubuntu:20.04-demo .

run:
	@docker run --rm -d --cap-add sys_ptrace -p127.0.0.1:2222:22 -v /home/wayne/src/github.com/gaia-platform/GaiaPlatform/scratch/wayne/hack:/home/user/hack --name demo ubuntu:20.04-demo

stop:
	@docker stop demo

exec:
	@docker exec -it -u user demo bash

ssh:
	@echo "Use 'password' as password."
	@ssh user@localhost -p2222
