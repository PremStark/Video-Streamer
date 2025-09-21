# GStreamer C++ Camera Streamer and Recorder

This is a simple C++ application that demonstrates how to use GStreamer to capture video from a camera, display it on the screen, and simultaneously save the stream to a file using the modern AV1 codec.

## Overview

The application builds a GStreamer pipeline that does the following:

1.  **Captures Video**: Uses the `autovideosrc` element to automatically detect and capture video from a connected camera.
2.  **Splits the Stream**: Uses the `tee` element to split the video stream into two identical paths.
3.  **Display Path**:
      * One path sends the video frames to a `queue`.
      * `videoconvert` ensures the video format is compatible with the display.
      * `autovideosink` automatically chooses the correct video sink to display the stream in a window.
4.  **Recording Path**:
      * The second path also sends frames to a `queue`.
      * `videoconvert` prepares the frames for encoding.
      * `av1enc` encodes the video into the high-efficiency AV1 format.
      * `matroskamux` muxes the encoded video into a Matroska (.mkv) container.
      * `filesink` writes the resulting data to a file named `recording.mkv`.

## Why use AV1?

AV1 (AOMedia Video 1) is a modern, open, and royalty-free video coding format designed for video transmissions over the Internet. We've chosen it for this project for a few key reasons:

  * **Superior Compression**: AV1 offers significantly better compression efficiency than older codecs like H.264 (used by `x264enc`). This means you can achieve the same video quality with a smaller file size, or higher quality for the same file size. This is great for saving disk space.
  * **Royalty-Free**: Being royalty-free makes it an attractive option for developers and companies, as it doesn't require licensing fees. This has led to its rapid adoption by major tech companies.
  * **Future-Proof**: It is developed by the Alliance for Open Media (AOMedia), a consortium that includes industry leaders like Google, Amazon, Apple, Microsoft, and Netflix. This strong backing ensures its continued development and wide support across browsers and devices.

## Dependencies

You need to have GStreamer and its development libraries installed on your system. You will also likely need a specific plugin for AV1 encoding, such as the one based on SVT-AV1.

### On Debian/Ubuntu:

```bash
sudo apt-get update
sudo apt-get install build-essential libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-svt-av1
```

### On Fedora:

```bash
sudo dnf install gstreamer1-devel gstreamer1-plugins-base-devel gstreamer1-plugins-good gstreamer1-plugins-bad-free gstreamer1-plugins-ugly-free gstreamer1-plugins-good-extras gstreamer1-plugins-bad-free-extras gstreamer1-libav
# You may need to enable a repository like RPM Fusion to find the SVT-AV1 plugin
sudo dnf install gstreamer1-plugin-svt-av1
sudo dnf groupinstall "Development Tools" "Development Libraries"
```

### On macOS (using Homebrew):

```bash
brew install gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad gst-plugins-ugly gst-libav svt-av1
```

## How to Compile

You can compile the program using `g++` and `pkg-config` to get the necessary GStreamer flags.

Open your terminal in the project directory and run:

```bash
g++ main.cpp -o gstreamer_recorder $(pkg-config --cflags --libs gstreamer-1.0)
```

This command will create an executable file named `gstreamer_recorder`.

## How to Run

Simply execute the compiled program from your terminal:

```bash
./gstreamer_recorder
```

When you run it, you should see two things happen:

1.  A window will pop up showing the live feed from your camera.
2.  A file named `recording.mkv` will be created in the same directory, containing the recorded video.

To stop the application and the recording, press `Ctrl+C` in the terminal.
