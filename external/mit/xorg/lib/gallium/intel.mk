# $NetBSD$

# Mesa's Intel support libraries.  Upstream builds these as separate Meson
# static libraries; the NetBSD Xorg build folds them into libgallium.

.if ${BUILD_IRIS} == 1 || ${BUILD_CROCUS} == 1

CPPFLAGS+=	-DSUPPORT_INTEL_INTEGRATED_GPUS \
		-I${X11SRCDIR.Mesa}/src/intel \
		-I${X11SRCDIR.Mesa}/../src/intel \
		-I${X11SRCDIR.Mesa}/src/intel/blorp \
		-I${X11SRCDIR.Mesa}/../src/intel/blorp \
		-I${X11SRCDIR.Mesa}/src/intel/common \
		-I${X11SRCDIR.Mesa}/src/intel/compiler \
		-I${X11SRCDIR.Mesa}/../src/intel/compiler \
		-I${X11SRCDIR.Mesa}/src/intel/compiler/brw \
		-I${X11SRCDIR.Mesa}/../src/intel/compiler/brw \
		-I${X11SRCDIR.Mesa}/src/intel/compiler/elk \
		-I${X11SRCDIR.Mesa}/../src/intel/compiler/elk \
		-I${X11SRCDIR.Mesa}/src/intel/compiler/jay \
		-I${X11SRCDIR.Mesa}/../src/intel/compiler/jay \
		-I${X11SRCDIR.Mesa}/src/intel/decoder \
		-I${X11SRCDIR.Mesa}/src/intel/dev \
		-I${X11SRCDIR.Mesa}/../src/intel/dev \
		-I${X11SRCDIR.Mesa}/src/intel/ds \
		-I${X11SRCDIR.Mesa}/../src/intel/ds \
		-I${X11SRCDIR.Mesa}/src/intel/genxml \
		-I${X11SRCDIR.Mesa}/../src/intel/genxml \
		-I${X11SRCDIR.Mesa}/src/intel/isl \
		-I${X11SRCDIR.Mesa}/../src/intel/isl \
		-I${X11SRCDIR.Mesa}/src/intel/perf \
		-I${X11SRCDIR.Mesa}/../src/intel/perf

.for _intel_dir in blorp common compiler compiler/brw compiler/elk \
	compiler/jay decoder dev ds genxml isl mda perf
.PATH:	${X11SRCDIR.Mesa}/src/intel/${_intel_dir}
.PATH:	${X11SRCDIR.Mesa}/../src/intel/${_intel_dir}
.endfor

INTEL_DEV_SOURCES= \
	i915_intel_device_info.c \
	intel_debug.c \
	intel_device_info.c \
	intel_hwconfig.c \
	intel_kmd.c \
	xe_intel_device_info.c \
	intel_wa.c

INTEL_COMMON_SOURCES= \
	i915_intel_engine.c \
	i915_intel_gem.c \
	intel_aux_map.c \
	intel_bind_timeline.c \
	intel_common.c \
	intel_compute_slm.c \
	intel_debug_identifier.c \
	common_intel_engine.c \
	intel_gem.c \
	intel_l3_config.c \
	intel_measure.c \
	intel_sample_positions.c \
	intel_urb_config.c \
	intel_uuid.c \
	xe_intel_device_query.c \
	xe_intel_engine.c \
	xe_intel_gem.c \
	xe_intel_queue.c

INTEL_ISL_SOURCES= \
	isl.c \
	isl_aux_info.c \
	isl_drm.c \
	isl_format.c \
	isl_storage_image.c \
	isl_format_layout.c \
	isl_gfx4.c \
	isl_gfx6.c \
	isl_gfx7.c \
	isl_gfx8.c \
	isl_gfx9.c \
	isl_gfx12.c \
	isl_gfx20.c \
	isl_tiled_memcpy_normal.c \
	isl_tiled_memcpy_sse41.c

INTEL_BLORP_SOURCES= \
	blorp.c \
	blorp_blit.c \
	blorp_clear.c \
	blorp_shaders.cpp

INTEL_COMPILER_NIR_SOURCES= \
	intel_nir.c \
	intel_nir_blockify_uniform_loads.c \
	intel_nir_clamp_image_1d_2d_array_sizes.c \
	intel_nir_clamp_per_vertex_loads.c \
	intel_nir_lower_non_uniform_barycentric_at_sample.c \
	intel_nir_lower_non_uniform_resource_intel.c \
	intel_nir_lower_printf.c \
	intel_nir_lower_shading_rate_output.c \
	intel_nir_lower_sparse.c \
	intel_nir_opt_peephole_ffma.c \
	intel_nir_opt_peephole_imul32x16.c \
	intel_nir_tcs_workarounds.c

INTEL_DECODER_SOURCES= \
	intel_batch_decoder.c \
	intel_decoder.c

INTEL_PERF_SOURCES= \
	i915_intel_perf.c \
	intel_perf.c \
	intel_perf_common.c \
	intel_perf_mdapi.c \
	intel_perf_query.c \
	xe_intel_perf.c \
	intel_perf_metrics.c

INTEL_DS_SOURCES= \
	intel_driver_ds.cc \
	intel_tracepoints.c

INTEL_MDA_SOURCES= \
	debug_archiver.c \
	mda_match.c \
	slice.c \
	tar.c

SRCS+=	${INTEL_DEV_SOURCES} ${INTEL_COMMON_SOURCES} \
	${INTEL_ISL_SOURCES} ${INTEL_BLORP_SOURCES} \
	${INTEL_COMPILER_NIR_SOURCES} ${INTEL_DECODER_SOURCES} \
	${INTEL_PERF_SOURCES} ${INTEL_DS_SOURCES} ${INTEL_MDA_SOURCES}

# Files with colliding basenames need unique object names in the mega-library.
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/dev/i915/intel_device_info.c i915_intel_device_info.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/dev/xe/intel_device_info.c xe_intel_device_info.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/common/i915/intel_engine.c i915_intel_engine.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/common/i915/intel_gem.c i915_intel_gem.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/common/xe/intel_device_query.c xe_intel_device_query.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/common/xe/intel_engine.c xe_intel_engine.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/common/xe/intel_gem.c xe_intel_gem.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/common/xe/intel_queue.c xe_intel_queue.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/common/intel_engine.c common_intel_engine.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/perf/i915/intel_perf.c i915_intel_perf.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/perf/xe/intel_perf.c xe_intel_perf.c

# ISL emits one copy of these entry points for each supported hardware version.
ISL_GENS=	40 50 60 70 75 80 90 110 120 125 200 300 350
ISL_GEN_SOURCES=	isl_emit_cpb.c isl_emit_depth_stencil.c isl_surface_state.c
.for _gen in ${ISL_GENS}
. for _src in ${ISL_GEN_SOURCES}
SRCS+=		${_gen}_${_src}
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/isl/${_src} ${_gen}_${_src}
CPPFLAGS.${_gen}_${_src}+=	-DGFX_VERx10=${_gen}
. endfor
.endfor

# Keep the generic implementation and the optimized x86_64 implementation.
CPPFLAGS.isl.c+=		-DUSE_SSE41
COPTS.isl_tiled_memcpy_sse41.c+=	-msse4.1

.if ${BUILD_IRIS} == 1

INTEL_BRW_SOURCES= \
	brw_analysis.cpp \
	brw_analysis_def.cpp \
	brw_analysis_liveness.cpp \
	brw_analysis_performance.cpp \
	brw_builder.cpp \
	brw_cfg.cpp \
	brw_compile_bs.cpp \
	brw_compile_cs.cpp \
	brw_compile_fs.cpp \
	brw_compile_gs.cpp \
	brw_compile_mesh.cpp \
	brw_compile_tcs.cpp \
	brw_compile_tes.cpp \
	brw_compile_vs.cpp \
	brw_compiler.c \
	brw_disasm.c \
	brw_disasm_info.cpp \
	brw_eu.c \
	brw_eu_compact.c \
	brw_eu_emit.c \
	brw_eu_validate.c \
	brw_from_nir.cpp \
	brw_generator.cpp \
	brw_inst.cpp \
	brw_load_reg.cpp \
	brw_lower.cpp \
	brw_lower_dpas.cpp \
	brw_lower_fill_spill.cpp \
	brw_lower_integer_multiplication.cpp \
	brw_lower_logical_sends.cpp \
	brw_lower_pack.cpp \
	brw_lower_regioning.cpp \
	brw_lower_scoreboard.cpp \
	brw_lower_simd_width.cpp \
	brw_lower_subgroup_ops.cpp \
	brw_nir.c \
	brw_nir_fence_shared_stores.c \
	brw_nir_lower_alpha_to_coverage.c \
	brw_nir_lower_cooperative_matrix.c \
	brw_nir_lower_cs_intrinsics.c \
	brw_nir_lower_fs_barycentrics.c \
	brw_nir_lower_fs_load_output.c \
	brw_nir_lower_immediate_offsets.c \
	brw_nir_lower_intersection_shader.c \
	brw_nir_lower_ray_queries.c \
	brw_nir_lower_rt_intrinsics.c \
	brw_nir_lower_rt_intrinsics_pre_trace.c \
	brw_nir_lower_sample_index_in_coord.c \
	brw_nir_lower_shader_calls.c \
	brw_nir_lower_storage_image.c \
	brw_nir_lower_texel_address.c \
	brw_nir_lower_texture.c \
	brw_nir_opt_divergent_atomics.c \
	brw_nir_opt_fsat.c \
	brw_nir_rt.c \
	brw_nir_wa_18019110168.c \
	brw_opt.cpp \
	brw_opt_address_reg_load.cpp \
	brw_opt_algebraic.cpp \
	brw_opt_bank_conflicts.cpp \
	brw_opt_cmod_propagation.cpp \
	brw_opt_cmp_flag_destination.cpp \
	brw_opt_combine_constants.cpp \
	brw_opt_copy_propagation.cpp \
	brw_opt_cse.cpp \
	brw_opt_dead_code_eliminate.cpp \
	brw_opt_fill_spill.cpp \
	brw_opt_register_coalesce.cpp \
	brw_opt_saturate_propagation.cpp \
	brw_opt_txf_combiner.cpp \
	brw_opt_virtual_grfs.cpp \
	brw_packed_float.c \
	brw_print.cpp \
	brw_reg.cpp \
	brw_reg_allocate.cpp \
	brw_reg_type.c \
	brw_sampler.c \
	brw_schedule_instructions.cpp \
	brw_shader.cpp \
	brw_simd_selection.cpp \
	brw_thread_payload.cpp \
	brw_validate.cpp \
	brw_vue_map.c \
	brw_workaround.cpp \
	brw_nir_lower_fsign.c \
	brw_nir_workarounds.c \
	brw_device_sha1_gen.c \
	intel_batch_decoder_brw.c \
	blorp_brw.c

INTEL_JAY_SOURCES= \
	jay_assign_flags.c \
	jay_from_nir.c \
	jay_insert_fp_mode.c \
	jay_liveness.c \
	jay_lower_post_ra.c \
	jay_lower_pre_ra.c \
	jay_lower_scoreboard.c \
	jay_lower_spill.c \
	jay_nir.c \
	jay_opt_control_flow.c \
	jay_opt_dead_code.c \
	jay_opt_propagate.c \
	jay_print.c \
	jay_prog_data.c \
	jay_register_allocate.c \
	jay_repair_ssa.c \
	jay_simd_width.c \
	jay_spill.c \
	jay_to_binary.c \
	jay_validate.c \
	jay_validate_ra.c \
	jay_nir_algebraic.c \
	jay_opcodes.c

SRCS+=	${INTEL_BRW_SOURCES} ${INTEL_JAY_SOURCES}

.endif

.if ${BUILD_CROCUS} == 1

INTEL_ELK_SOURCES= \
	elk_cfg.cpp \
	elk_clip_line.c \
	elk_clip_point.c \
	elk_clip_tri.c \
	elk_clip_unfilled.c \
	elk_clip_util.c \
	elk_compile_clip.c \
	elk_compile_ff_gs.c \
	elk_compile_sf.c \
	elk_compiler.c \
	elk_dead_control_flow.cpp \
	elk_debug_recompile.c \
	elk_disasm.c \
	elk_disasm_info.c \
	elk_eu.c \
	elk_eu_compact.c \
	elk_eu_emit.c \
	elk_eu_util.c \
	elk_eu_validate.c \
	elk_fs.cpp \
	elk_fs_bank_conflicts.cpp \
	elk_fs_cmod_propagation.cpp \
	elk_fs_combine_constants.cpp \
	elk_fs_copy_propagation.cpp \
	elk_fs_cse.cpp \
	elk_fs_dead_code_eliminate.cpp \
	elk_fs_generator.cpp \
	elk_fs_live_variables.cpp \
	elk_fs_lower_pack.cpp \
	elk_fs_lower_regioning.cpp \
	elk_fs_nir.cpp \
	elk_fs_reg_allocate.cpp \
	elk_fs_register_coalesce.cpp \
	elk_fs_saturate_propagation.cpp \
	elk_fs_sel_peephole.cpp \
	elk_fs_thread_payload.cpp \
	elk_fs_validate.cpp \
	elk_fs_visitor.cpp \
	elk_gfx6_gs_visitor.cpp \
	elk_interpolation_map.c \
	elk_ir_performance.cpp \
	elk_lower_logical_sends.cpp \
	elk_nir.c \
	elk_nir_analyze_boolean_resolves.c \
	elk_nir_analyze_ubo_ranges.c \
	elk_nir_attribute_workarounds.c \
	elk_nir_lower_alpha_to_coverage.c \
	elk_nir_lower_cs_intrinsics.c \
	elk_nir_lower_storage_image.c \
	elk_nir_options.c \
	elk_packed_float.c \
	elk_predicated_break.cpp \
	elk_reg_type.c \
	elk_schedule_instructions.cpp \
	elk_shader.cpp \
	elk_simd_selection.cpp \
	elk_vec4.cpp \
	elk_vec4_cmod_propagation.cpp \
	elk_vec4_copy_propagation.cpp \
	elk_vec4_cse.cpp \
	elk_vec4_dead_code_eliminate.cpp \
	elk_vec4_generator.cpp \
	elk_vec4_gs_nir.cpp \
	elk_vec4_gs_visitor.cpp \
	elk_vec4_live_variables.cpp \
	elk_vec4_nir.cpp \
	elk_vec4_reg_allocate.cpp \
	elk_vec4_surface_builder.cpp \
	elk_vec4_tcs.cpp \
	elk_vec4_tes.cpp \
	elk_vec4_visitor.cpp \
	elk_vec4_vs_visitor.cpp \
	elk_vue_map.c \
	elk_nir_trig_workarounds.c \
	intel_batch_decoder_elk.c \
	blorp_elk.c

SRCS+=	${INTEL_ELK_SOURCES}

.endif

.endif
