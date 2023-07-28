# ---- Builder Image ----
FROM debian:11 as transcribe_builder

WORKDIR /

# Create appuser
ENV USER=appuser
ENV UID=10001
# See https://stackoverflow.com/a/55757473/12429735RUN
RUN adduser --disabled-password --gecos "" \
    --home "/nonexistent" --shell "/sbin/nologin" --no-create-home \
    --uid "${UID}" "${USER}"

# Set environment variables for your web server
ENV SERVER_PORT=8080

# Install necessary build dependencies
RUN apt-get update && apt-get -y --no-install-recommends install \
    build-essential clang cmake gdb libboost-all-dev \
    ffmpeg libavcodec-dev libavformat-dev libavutil-dev libswresample-dev libswscale-dev \
    libavfilter-dev libavdevice-dev libbz2-dev libmp3lame-dev libopus-dev libvorbis-dev

# Copy the C++ web server source code into the container
COPY ./ /app

# Set the working directory
WORKDIR /app

# Build the C++ web server
RUN cmake . && make

RUN chmod +x /app/bin/transcriber

# ---- Final Image ----
FROM debian:11-slim

WORKDIR /home/nonroot

COPY --from=transcribe_builder /etc/passwd /etc/passwd
COPY --from=transcribe_builder /etc/group /etc/group

# Copy only the necessary files from the builder image
COPY --from=transcribe_builder /app/bin/transcriber /home/nonroot/transcriber

# Use an unprivileged user.
USER appuser:appuser

# Set environment variables for your web server
ENV SERVER_PORT=8080

# Expose the port your web server will be listening on
EXPOSE ${SERVER_PORT}

# Start the web server when the container runs
CMD ["/home/nonroot/transcriber"]
