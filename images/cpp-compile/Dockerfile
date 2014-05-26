FROM ubuntu
ADD init.sh /imageinit/init.sh
ADD entrypoint.sh /imageinit/entrypoint.sh
ADD compile.sh /imageinit/compile.sh
RUN chmod +x /imageinit/*.sh
RUN /imageinit/init.sh
ENTRYPOINT /imageinit/entrypoint.sh
