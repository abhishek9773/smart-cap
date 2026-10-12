# Current limitations

The Smart Cap is an embedded camera and image-preprocessing prototype. The
complete assistive system is not yet finished.

## Not implemented or validated

- AI inference or object recognition (including people, vehicles, or animals)
- Object distance estimation, collision prediction, or motion tracking
- Speech synthesis, spoken directions, or other user alert interface
- End-to-end navigation decisions or obstacle avoidance
- Clinical, accessibility, safety, or long-duration field evaluation
- Production packaging, power-life characterization, or release qualification

The board's OLED is checked over I²C and receives a test-pattern write, but
the available evidence does not claim that its output was visually inspected.

## Test-result limits

The approximately 30-second run with 270 successful images and zero reported
processing failures is one recorded observation. The approximately 9 images
per second is observed preprocessing throughput, not measured camera FPS. It
must not be generalized into a stability or product-performance guarantee.
