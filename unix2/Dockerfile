FROM alpine:latest
RUN apk add --no-cache util-linux
WORKDIR /app
COPY worker.sh /app/worker.sh
RUN chmod +x /app/worker.sh
CMD ["/app/worker.sh"]
