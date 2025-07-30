func s_m4 make_orthographic(float left, float right, float bottom, float top, float near, float far);
func s_quaternion dir_to_quaternion(s_v3 dir);
func s_v3 v3_rotate(s_v3 v, s_quaternion q);
func s_m4 quaternion_to_m4(s_quaternion left);
func s_v4 v4_multiply_m4(s_v4 v, s_m4 m);
func s_quaternion quaternion_from_axis_angle(s_v3 axis, float angle);
func s_quaternion quaternion_divide_f(s_quaternion Left, float Dividend);
func s_quaternion quaternion_divide(s_quaternion Left, float Right);
func float quaternion_dot(s_quaternion Left, s_quaternion Right);
func s_quaternion quaternion_normalized(s_quaternion Left);
template <typename t>
func void swap(t* a, t* b);
func int circular_index(int index, int size);
func s_ray get_camera_ray(s_v3 cam_pos, s_m4 view, s_m4 projection, s_v2 mouse, s_v2 world_size);
func s_m4 m4_inverse(const s_m4 m);
func s_v3 ray_at_y(s_ray ray, float y);
func float lerp(float a, float b, float t);
func s_v3 lerp_v3(s_v3 a, s_v3 b, float t);
func float range_lerp(float input_val, float input_start, float input_end, float output_start, float output_end);
template <typename t>
func t min(t a, t b);
func s_v3 v3_set_mag(s_v3 v, float len);
func float smoothstep(float edge0, float edge1, float x);
func float ilerp(float start, float end, float val);
template <typename t>
func void at_most_ptr(t* ptr, t max_val);
template <typename t>
func void at_least_ptr(t* ptr, t min_val);
func int ceilfi(float x);
func int roundfi(float x);
func float v3_distance(s_v3 a, s_v3 b);
func float go_towards(float from, float to, float amount);
func s_v3 go_towards(s_v3 from, s_v3 to, float amount);
func float sign(float x);
func b8 sphere_vs_sphere(s_v3 pos1, float r1, s_v3 pos2, float r2);
template <typename t>
func t max(t a, t b);
func float ilerp_clamp(float start, float end, float value);
func float handle_advanced_easing(float x, float x_start, float x_end);
func float ease_in_expo(float x);
func float ease_linear(float x);
func float ease_in_quad(float x);
func float ease_out_quad(float x);
func float ease_out_expo(float x);
func float ease_out_elastic(float x);
func float ease_out_elastic2(float x);
func float ease_out_back(float x);
func float lerp_snap(float a, float b, float t, float max_diff);
func s_v2 lerp_snap(s_v2 a, s_v2 b, float t);
func s_v2 lerp_snap(s_v2 a, s_v2 b, float t, s_v2 max_diff);
func float v2_distance(s_v2 a, s_v2 b);
func float v2_length(s_v2 a);
func s_v2 v2_from_angle(float angle);
func s_quaternion quaternion_multiply(s_quaternion second, s_quaternion first);
