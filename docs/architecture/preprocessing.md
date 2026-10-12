# Image preprocessing

The preprocessor is implemented in
`components/image_preprocessor/` using Espressif's `esp_jpeg` decoder.

## Decode, scale, resize, convert

1. The preprocessor inspects JPEG dimensions.
2. It selects a supported JPEG decode scale (1/8, 1/4, 1/2, or full-size)
   within the configured maximum decode-pixel budget.
3. It decodes to an RGB888 intermediate buffer.
4. It resizes to the requested output dimensions using nearest-neighbor
   sampling.
5. It converts output pixels to RGB888, RGB565, or grayscale.

When the selected decoded dimensions are smaller than the requested output,
nearest-neighbor sampling expands them to the configured output size. The
decoder scale selection is bounded by `max_decode_pixels`.

## Validation configuration

The application currently requests 320 × 240 QVGA JPEG and produces a
160 × 120 RGB565 output image. The output has a 320-byte row stride and
38,400 bytes total.

Other supported output sizes/formats can be selected through the preprocessor
configuration, provided the configured maximums accommodate them.

## API usage shape

```c
camera_pipeline_handle_t pipeline = NULL;
image_preprocessed_image_t image = {0};

esp_err_t err = camera_pipeline_create(&config, &pipeline);
if (err == ESP_OK) {
    err = camera_pipeline_process_next(pipeline, &image);
    if (err == ESP_OK) {
        /* Consume image.data before requesting another frame. */
    }
    esp_err_t cleanup_err = camera_pipeline_destroy(pipeline);
    (void)cleanup_err;
}
```

The snippet illustrates API ordering; production callers should handle each
error explicitly. The image buffer belongs to the preprocessor and is reused.
