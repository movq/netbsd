#	$NetBSD: libglsl.mk,v 1.8 2023/11/22 17:48:35 rjs Exp $

LIBGLSL_GENERATED_CXX_FILES = \
	glsl_parser.cpp \
	glsl_lexer.cpp

#COPTS.glsl_lexer.cpp+=	-Wno-deprecated-register
COPTS.vtn_glsl450.c+=	${${ACTIVE_CC} == "clang":? -Wno-error=enum-conversion :}

CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/compiler \
		-I${X11SRCDIR.Mesa}/../src/compiler \
		-I${X11SRCDIR.Mesa}/src/compiler/nir \
		-I${X11SRCDIR.Mesa}/../src/compiler/nir \
		-I${X11SRCDIR.Mesa}/src/compiler/glsl \
		-I${X11SRCDIR.Mesa}/../src/compiler/glsl \
		-I${X11SRCDIR.Mesa}/src/compiler/glsl/glcpp \
		-I${X11SRCDIR.Mesa}/../src/compiler/glsl/glcpp \
		-I${X11SRCDIR.Mesa}/src/compiler/spirv \
		-I${X11SRCDIR.Mesa}/../src/compiler/spirv

# Core compiler type and enum helpers are a separate Mesa library now.
.PATH:		${X11SRCDIR.Mesa}/src/compiler
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/../src/compiler/builtin_types.c compiler_builtin_types.c
SRCS+=		glsl_types.c shader_enums.c compiler_builtin_types.c

LIBGLSL_FILES = \
	ast_array_index.cpp \
	ast_expr.cpp \
	ast_function.cpp \
	ast_to_hir.cpp \
	ast_type.cpp \
	builtin_functions.cpp \
	builtin_types.cpp \
	builtin_variables.cpp \
	glsl_parser_extras.cpp \
	glsl_symbol_table.cpp \
	glsl_to_nir.cpp \
	hir_field_selection.cpp \
	ir_basic_block.cpp \
	ir_builder.cpp \
	ir_clone.cpp \
	ir_constant_expression.cpp \
	ir.cpp \
	ir_expression_flattening.cpp \
	ir_function_detect_recursion.cpp \
	ir_function.cpp \
	ir_hierarchical_visitor.cpp \
	ir_hv_accept.cpp \
	ir_print_visitor.cpp \
	ir_rvalue_visitor.cpp \
	ir_validate.cpp \
	ir_variable_refcount.cpp \
	linker_util.cpp \
	lower_builtins.cpp \
	lower_instructions.cpp \
	lower_jumps.cpp \
	lower_mat_op_to_vec.cpp \
	lower_packing_builtins.cpp \
	lower_precision.cpp \
	lower_subroutine.cpp \
	lower_vec_index_to_cond_assign.cpp \
	lower_vector_derefs.cpp \
	opt_algebraic.cpp \
	opt_dead_builtin_variables.cpp \
	opt_dead_code.cpp \
	opt_flatten_nested_if_blocks.cpp \
	opt_function_inlining.cpp \
	opt_if_simplification.cpp \
	opt_minmax.cpp \
	opt_rebalance_tree.cpp \
	opt_tree_grafting.cpp \
	propagate_invariance.cpp \
	string_to_uint_map.cpp \
	serialize.cpp \
	shader_cache.cpp \
	gl_nir_detect_function_recursion.c \
	gl_nir_lower_atomics.c \
	gl_nir_lower_images.c \
	gl_nir_lower_blend_equation_advanced.c \
	gl_nir_lower_buffers.c \
	gl_nir_lower_discard_flow.c \
	gl_nir_lower_named_interface_blocks.c \
	gl_nir_lower_packed_varyings.c \
	gl_nir_lower_samplers.c \
	gl_nir_lower_samplers_as_deref.c \
	gl_nir_lower_xfb_varying.c \
	gl_nir_link_atomics.c \
	gl_nir_link_functions.c \
	gl_nir_link_interface_blocks.c \
	gl_nir_link_uniform_blocks.c \
	gl_nir_link_uniform_initializers.c \
	gl_nir_link_uniforms.c \
	gl_nir_link_varyings.c \
	gl_nir_link_xfb.c \
	gl_nir_linker.c

# XXX
.if ${MACHINE} == "vax"
COPTS.ir_constant_expression.cpp+=	-O0
COPTS.ir.cpp+=	-O0
COPTS.nir_constant_expressions.c+=	-O0
.endif

LIBGLCPP_GENERATED_FILES = \
	glcpp-lex.c \
	glcpp-parse.c

LIBGLCPP_FILES = \
	pp.c

NIR_GENERATED_FILES = \
	nir_opt_algebraic.c \
	nir_opcodes.c \
	nir_constant_expressions.c \
	nir_intrinsics.c

BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/compiler/nir/nir.c nir_nir.c

NIR_FILES = \
	nir_nir.c \
	nir_builder.c \
	nir_builtin_builder.c \
	nir_clip_cull_distance_io_utils.c \
	nir_clone.c \
	nir_control_flow.c \
	nir_deref.c \
	nir_divergence_analysis.c \
	nir_dominance.c \
	nir_dominance_lca.c \
	nir_downgrade_pls_vars.c \
	nir_fixup_is_exported.c \
	nir_format_convert.c \
	nir_from_ssa.c \
	nir_functions.c \
	nir_gather_info.c \
	nir_gather_output_deps.c \
	nir_gather_tcs_info.c \
	nir_gather_types.c \
	nir_gather_xfb_info.c \
	nir_opt_group_loads.c \
	nir_gs_count_vertices.c \
	nir_inline_sysval.c \
	nir_inline_uniforms.c \
	nir_instr_set.c \
	nir_io_add_xfb_info.c \
	nir_legacy.c \
	nir_linking_helpers.c \
	nir_liveness.c \
	nir_loop_analyze.c \
	nir_lower_alu.c \
	nir_lower_alu_width.c \
	nir_lower_alpha.c \
	nir_lower_amul.c \
	nir_lower_array_deref_of_vec.c \
	nir_lower_atomics_to_ssbo.c \
	nir_lower_bitmap.c \
	nir_lower_blend.c \
	nir_lower_bool_to_float.c \
	nir_lower_bool_to_int32.c \
	nir_lower_calls_to_builtins.c \
	nir_lower_cl_images.c \
	nir_lower_clamp_color_outputs.c \
	nir_lower_clip.c \
	nir_lower_clip_disable.c \
	nir_lower_clip_halfz.c \
	nir_lower_const_arrays_to_uniforms.c \
	nir_lower_continue_constructs.c \
	nir_lower_convert_alu_types.c \
	nir_lower_cooperative_matrix.c \
	nir_lower_variable_initializers.c \
	nir_lower_discard_if.c \
	nir_lower_double_ops.c \
	nir_lower_explicit_io.c \
	nir_lower_fb_read.c \
	nir_lower_flatshade.c \
	nir_lower_floats.c \
	nir_lower_flrp.c \
	nir_lower_fp16_conv.c \
	nir_lower_fragcoord_wtrans.c \
	nir_lower_frag_coord_to_pixel_coord.c \
	nir_lower_fragcolor.c \
	nir_lower_frexp.c \
	nir_lower_global_vars_to_local.c \
	nir_lower_goto_ifs.c \
	nir_lower_gs_intrinsics.c \
	nir_lower_halt_to_return.c \
	nir_lower_helper_writes.c \
	nir_lower_load_const_to_scalar.c \
	nir_lower_locals_to_regs.c \
	nir_lower_idiv.c \
	nir_lower_image.c \
	nir_lower_image_atomics_to_global.c \
	nir_lower_indirect_derefs_to_if_else_trees.c \
	nir_lower_input_attachments.c \
	nir_lower_int64.c \
	nir_lower_interpolation.c \
	nir_lower_int_to_float.c \
	nir_lower_io.c \
	nir_lower_io_array_vars_to_elements.c \
	nir_lower_io_indirect_loads.c \
	nir_lower_io_vars_to_temporaries.c \
	nir_lower_io_to_scalar.c \
	nir_lower_io_vars_to_scalar.c \
	nir_lower_is_helper_invocation.c \
	nir_lower_multiview.c \
	nir_lower_mediump.c \
	nir_lower_mem_access_bit_sizes.c \
	nir_lower_memcpy.c \
	nir_lower_memory_model.c \
	nir_lower_non_uniform_access.c \
	nir_lower_packing.c \
	nir_lower_passthrough_edgeflags.c \
	nir_lower_patch_vertices.c \
	nir_lower_phis_to_scalar.c \
	nir_lower_pntc_ytransform.c \
	nir_lower_point_size.c \
	nir_lower_point_smooth.c \
	nir_lower_poly_line_smooth.c \
	nir_lower_printf.c \
	nir_lower_reg_intrinsics_to_ssa.c \
	nir_lower_readonly_images_to_tex.c \
	nir_lower_returns.c \
	nir_lower_robust_access.c \
	nir_lower_samplers.c \
	nir_lower_sample_shading.c \
	nir_lower_scratch.c \
	nir_lower_scratch_to_var.c \
	nir_lower_shader_calls.c \
	nir_lower_single_sampled.c \
	nir_lower_ssbo.c \
	nir_lower_subgroups.c \
	nir_lower_system_values.c \
	nir_lower_task_shader.c \
	nir_lower_terminate_to_demote.c \
	nir_lower_tess_coord_z.c \
	nir_lower_tex_shadow.c \
	nir_lower_tex.c \
	nir_lower_texcoord_replace.c \
	nir_lower_texcoord_replace_late.c \
	nir_lower_two_sided_color.c \
	nir_lower_undef_to_zero.c \
	nir_lower_vars_to_ssa.c \
	nir_lower_var_copies.c \
	nir_lower_vec_to_regs.c \
	nir_lower_vec3_to_vec4.c \
	nir_lower_view_index_to_device_index.c \
	nir_lower_viewport_transform.c \
	nir_lower_wpos_center.c \
	nir_lower_wpos_ytransform.c \
	nir_lower_wrmasks.c \
	nir_lower_bit_size.c \
	nir_lower_ubo_vec4.c \
	nir_lower_uniforms_to_ubo.c \
	nir_lower_workgroup_size.c \
	nir_lower_sysvals_to_varyings.c \
	nir_metadata.c \
	nir_mod_analysis.c \
	nir_move_output_stores_to_end.c \
	nir_move_vec_src_uses_to_dest.c \
	nir_normalize_cubemap_coords.c \
	nir_normalize_sin_cos.c \
	nir_opt_access.c \
	nir_opt_barriers.c \
	nir_opt_barycentric.c \
	nir_opt_call.c \
	nir_opt_clip_cull_const.c \
	nir_opt_combine_stores.c \
	nir_opt_comparison_pre.c \
	nir_opt_constant_folding.c \
	nir_opt_copy_prop_vars.c \
	nir_opt_copy_propagate.c \
	nir_opt_cse.c \
	nir_opt_dce.c \
	nir_opt_dead_cf.c \
	nir_opt_dead_write_vars.c \
	nir_opt_find_array_copies.c \
	nir_opt_fp_math_ctrl.c \
	nir_opt_frag_coord_to_pixel_coord.c \
	nir_opt_fragdepth.c \
	nir_opt_gcm.c \
	nir_opt_generate_bfi.c \
	nir_opt_idiv_const.c \
	nir_opt_if.c \
	nir_opt_intrinsics.c \
	nir_opt_large_constants.c \
	nir_opt_licm.c \
	nir_opt_load_skip_helpers.c \
	nir_opt_load_store_vectorize.c \
	nir_opt_loop.c \
	nir_opt_loop_unroll.c \
	nir_opt_memcpy.c \
	nir_opt_move.c \
	nir_opt_move_discards_to_top.c \
	nir_opt_move_to_top.c \
	nir_opt_mqsad.c \
	nir_opt_non_uniform_access.c \
	nir_opt_offsets.c \
	nir_opt_peephole_select.c \
	nir_opt_phi_precision.c \
	nir_opt_phi_to_bool.c \
	nir_opt_preamble.c \
	nir_opt_ray_queries.c \
	nir_opt_reassociate.c \
	nir_opt_reassociate_bfi.c \
	nir_opt_rematerialize_compares.c \
	nir_opt_remove_phis.c \
	nir_opt_shrink_stores.c \
	nir_opt_shrink_vectors.c \
	nir_opt_sink.c \
	nir_opt_undef.c \
	nir_opt_uniform_atomics.c \
	nir_opt_uniform_subgroup.c \
	nir_opt_uub.c \
	nir_opt_varyings.c \
	nir_opt_vectorize.c \
	nir_opt_vectorize_io.c \
	nir_opt_vectorize_io_vars.c \
	nir_passthrough_gs.c \
	nir_passthrough_tcs.c \
	nir_phi_builder.c \
	nir_print.c \
	nir_propagate_invariant.c \
	nir_range_analysis.c \
	nir_recompute_io_bases.c \
	nir_remove_dead_variables.c \
	nir_remove_outputs.c \
	nir_remove_tex_shadow.c \
	nir_repair_ssa.c \
	nir_scale_fdiv.c \
	nir_schedule.c \
	nir_search.c \
	nir_separate_merged_clip_cull_io.c \
	nir_serialize.c \
	nir_shader_bisect.c \
	nir_split_64bit_vec3_and_vec4.c \
	nir_split_conversions.c \
	nir_split_per_member_structs.c \
	nir_split_var_copies.c \
	nir_split_vars.c \
	nir_sweep.c \
	nir_to_lcssa.c \
	nir_trivialize_registers.c \
	nir_unlower_io_to_vars.c \
	nir_use_dominance.c \
	nir_validate.c \
	nir_worklist.c \
	nir_lower_atomics.c

SPIRV_GENERATED_FILES = \
	spirv_info.c \
	vtn_gather_types.c

SPIRV_FILES = \
	gl_spirv.c \
	spirv_to_nir.c \
	vtn_alu.c \
	vtn_amd.c \
	vtn_cfg.c \
	vtn_cmat.c \
	vtn_debug.c \
	vtn_glsl450.c \
	vtn_opencl.c \
	vtn_structured_cfg.c \
	vtn_subgroup.c \
	vtn_variables.c


.PATH:	${X11SRCDIR.Mesa}/src/compiler
.PATH:	${X11SRCDIR.Mesa}/src/compiler/glsl
.PATH:	${X11SRCDIR.Mesa}/../src/compiler/glsl
.PATH:	${X11SRCDIR.Mesa}/src/compiler/glsl/glcpp
.PATH:	${X11SRCDIR.Mesa}/../src/compiler/glsl/glcpp
.PATH:	${X11SRCDIR.Mesa}/src/compiler/nir
.PATH:	${X11SRCDIR.Mesa}/../src/compiler/nir
.PATH:	${X11SRCDIR.Mesa}/src/compiler/spirv
.PATH:	${X11SRCDIR.Mesa}/../src/compiler/spirv

SRCS+=	${LIBGLSL_GENERATED_CXX_FILES} \
	${LIBGLSL_FILES} \
	${LIBGLCPP_GENERATED_FILES} \
	${LIBGLCPP_FILES} \
	${NIR_GENERATED_FILES} \
	${NIR_FILES} \
	${SPIRV_GENERATED_FILES} \
	${SPIRV_FILES}
