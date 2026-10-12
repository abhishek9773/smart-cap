# Camera pipeline

`components/camera_pipeline/` connects the camera driver to the image
preprocessor. It exposes a demand-driven `camera_pipeline_process_next()`
operation rather than scheduling fixed-rate capture.

For each call, the pipeline:

1. Acquires a camera framebuffer.
2. Requires JPEG input and copies the JPEG into the preprocessor's bounded
   staging buffer.
3. Returns the camera-owned framebuffer.
4. Decodes and preprocesses the staged JPEG.
5. Returns metadata and a pointer to the preprocessor-owned output buffer.

If frame acquisition, input validation, or staging fails, the error is
returned. The camera buffer is released before a decode operation starts.

## Output lifetime

The output image's data pointer refers to a reusable preprocessor buffer.
Consumers must finish using or copying the image before requesting another
processed image from the same pipeline, because a later call reuses that
storage.

The application currently consumes images only for validation metrics. It does
not run inference, encode speech, or display the camera image.
