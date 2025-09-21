#include <gst/gst.h>
#include <iostream>

// Structure to hold all our information, so we can pass it to callbacks
typedef struct _CustomData {
  GstElement *pipeline;
  GstElement *source;
  GstElement *tee;
  GstElement *video_queue_display;
  GstElement *video_convert_display;
  GstElement *video_sink_display;
  GstElement *video_queue_record;
  GstElement *video_convert_record;
  GstElement *video_encode;
  GstElement *muxer;
  GstElement *file_sink;
  GMainLoop *loop;
} CustomData;

// Handler for the pad-added signal
static void pad_added_handler(GstElement *src, GstPad *pad, CustomData *data);

int main(int argc, char *argv[]) {
  CustomData data;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  gboolean terminate = FALSE;

  // Initialize GStreamer
  gst_init(&argc, &argv);

  // Create the elements
  // Use autovideosrc for automatic camera detection
  data.source = gst_element_factory_make("autovideosrc", "source");
  if (!data.source) {
    g_printerr("Not all elements could be created.\n");
    return -1;
  }

  data.tee = gst_element_factory_make("tee", "tee");

  // Display branch elements
  data.video_queue_display = gst_element_factory_make("queue", "video_queue_display");
  data.video_convert_display = gst_element_factory_make("videoconvert", "video_convert_display");
  // Use autovideosink for automatic display
  data.video_sink_display = gst_element_factory_make("autovideosink", "video_sink_display");

  // Recording branch elements
  data.video_queue_record = gst_element_factory_make("queue", "video_queue_record");
  data.video_convert_record = gst_element_factory_make("videoconvert", "video_convert_record_2");
  // Using av1enc for AV1 encoding
  data.video_encode = gst_element_factory_make("av1enc", "video_encode");
  // Using matroskamux for MKV container, which has better AV1 support
  data.muxer = gst_element_factory_make("matroskamux", "muxer");
  data.file_sink = gst_element_factory_make("filesink", "file_sink");

  // Create the empty pipeline
  data.pipeline = gst_pipeline_new("test-pipeline");

  if (!data.pipeline || !data.source || !data.tee ||
      !data.video_queue_display || !data.video_convert_display || !data.video_sink_display ||
      !data.video_queue_record || !data.video_convert_record || !data.video_encode ||
      !data.muxer || !data.file_sink) {
    g_printerr("Not all elements could be created.\n");
    return -1;
  }

  // Set the output file location
  g_object_set(data.file_sink, "location", "recording.mkv", NULL);

  // Build the pipeline
  gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.tee,
                   data.video_queue_display, data.video_convert_display, data.video_sink_display,
                   data.video_queue_record, data.video_convert_record, data.video_encode,
                   data.muxer, data.file_sink, NULL);

  // Link the source to the tee
  if (gst_element_link(data.source, data.tee) != TRUE) {
    g_printerr("Source and tee could not be linked.\n");
    gst_object_unref(data.pipeline);
    return -1;
  }

  // Link the display branch
  if (gst_element_link_many(data.video_queue_display, data.video_convert_display, data.video_sink_display, NULL) != TRUE) {
    g_printerr("Display branch elements could not be linked.\n");
    gst_object_unref(data.pipeline);
    return -1;
  }

  // Link the record branch
  if (gst_element_link_many(data.video_queue_record, data.video_convert_record, data.video_encode, data.muxer, data.file_sink, NULL) != TRUE) {
    g_printerr("Recording branch elements could not be linked.\n");
    gst_object_unref(data.pipeline);
    return -1;
  }
  
  // Manually link the tee to the two branches
  GstPadTemplate *tee_src_pad_template;
  GstPad *tee_display_pad;
  GstPad *tee_record_pad;
  GstPad *queue_display_sink_pad;
  GstPad *queue_record_sink_pad;

  tee_src_pad_template = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(data.tee), "src_%u");
  
  // Display pad
  tee_display_pad = gst_element_request_pad(data.tee, tee_src_pad_template, NULL, NULL);
  g_print("Obtained request pad %s for display branch.\n", gst_pad_get_name(tee_display_pad));
  queue_display_sink_pad = gst_element_get_static_pad(data.video_queue_display, "sink");
  
  // Record pad
  tee_record_pad = gst_element_request_pad(data.tee, tee_src_pad_template, NULL, NULL);
  g_print("Obtained request pad %s for record branch.\n", gst_pad_get_name(tee_record_pad));
  queue_record_sink_pad = gst_element_get_static_pad(data.video_queue_record, "sink");

  // Link them
  if (gst_pad_link(tee_display_pad, queue_display_sink_pad) != GST_PAD_LINK_OK ||
      gst_pad_link(tee_record_pad, queue_record_sink_pad) != GST_PAD_LINK_OK) {
    g_printerr("Tee could not be linked.\n");
    gst_object_unref(data.pipeline);
    return -1;
  }

  // Unreference the pads we got
  gst_object_unref(queue_display_sink_pad);
  gst_object_unref(queue_record_sink_pad);
  
  // Set the pipeline to the playing state
  ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Unable to set the pipeline to the playing state.\n");
    gst_object_unref(data.pipeline);
    return -1;
  }

  // Wait until error or EOS
  bus = gst_element_get_bus(data.pipeline);
  
  // Create a GLib Main Loop and connect it to the bus
  data.loop = g_main_loop_new(NULL, FALSE);

  do {
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        (GstMessageType)(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // Parse message
    if (msg != NULL) {
      GError *err;
      gchar *debug_info;

      switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR:
          gst_message_parse_error(msg, &err, &debug_info);
          g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
          g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
          g_clear_error(&err);
          g_free(debug_info);
          terminate = TRUE;
          break;
        case GST_MESSAGE_EOS:
          g_print("End-Of-Stream reached.\n");
          terminate = TRUE;
          break;
        case GST_MESSAGE_STATE_CHANGED:
          // We are only interested in state-changed messages from the pipeline
          if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data.pipeline)) {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
            g_print("Pipeline state changed from %s to %s:\n",
                gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
          }
          break;
        default:
          // We should not reach here
          g_printerr("Unexpected message received.\n");
          break;
      }
      gst_message_unref(msg);
    }
  } while (!terminate);

  // Free resources
  gst_element_release_request_pad(data.tee, tee_display_pad);
  gst_element_release_request_pad(data.tee, tee_record_pad);
  gst_object_unref(tee_display_pad);
  gst_object_unref(tee_record_pad);

  gst_object_unref(bus);
  gst_element_set_state(data.pipeline, GST_STATE_NULL);
  gst_object_unref(data.pipeline);
  return 0;
}

