
#ifdef __cplusplus
extern "C" {
#endif

  void yuv420_2_rgb8888_neon( 
    uint8_t *dst_ptr, 
    const uint8_t *y_ptr,
    const uint8_t *u_ptr,
    const uint8_t *v_ptr,
    int width,
    int height,
    int y_pitch,
    int uv_pitch,
    int rgb_pitch);

  void yuv422_2_rgb8888_neon(
    uint8_t *dst_ptr,
    const uint8_t *y_ptr,
    const uint8_t *u_ptr,
    const uint8_t *v_ptr,
    int width,
    int height,
    int y_pitch,
    int uv_pitch,
    int rgb_pitch
  );

#ifdef __cplusplus
}
#endif
