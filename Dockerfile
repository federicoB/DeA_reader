FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y radlib-dev build-essential cmake libusb-dev libusb-1.0-0 libusb-1.0-0-dev && \
    apt-get clean

WORKDIR /app

# Copy source files
COPY CMakeLists.txt deaprotocol.c deaprotocol.h hidapi.h \
     main.c sysdefs.h wvutils.c README.md ./

# Build
RUN cmake . && make

# Default command
CMD ["./dea_reader"]