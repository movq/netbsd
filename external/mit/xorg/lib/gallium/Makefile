# $NetBSD: Makefile,v 1.64 2025/11/15 06:53:16 mrg Exp $

# Link the gallium mega driver.  This is shared by the DRI, GLX, EGL,
# and GBM frontends; the DRI driver names installed below are aliases.

NOLINKLIB=	yes
NOSTATICLIB=	yes
NOPROFILE=	yes
NOPICINSTALL=	yes
LIBISCXX=     yes

NOLINT=		# Lots of "Unsupported platform" due to undefined __GNUC__

.include <bsd.own.mk>

.include "../mesa-which.mk"

SHLIB_MAJOR=    0
SHLIB_MINOR=    0

LIB=		gallium
DRIDIR=		${X11USRLIBDIR}/modules/dri
DRIDEBUGDIR=	${DEBUGDIR}${X11USRLIBDIR}/modules/dri

LLVM_INCLUDE_OBJDIR!=	cd ${NETBSDSRCDIR}/external/apache2/llvm/include && ${PRINTOBJDIR}

CWARNFLAGS.clang += -Wno-atomic-alignment -Wno-unknown-warning-option -Wno-implicit-int-float-conversion -Wno-c99-designator
COPTS.nouveau.c+=	${${ACTIVE_CC} == "clang":? -Wno-error=gnu-variable-sized-type-not-at-end :}

CXXFLAGS+=	-std=c++17
CFLAGS+=	-std=gnu11
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/gallium/winsys/nouveau/drm \
		-I${X11SRCDIR.Mesa}/src/nouveau/drm \
		-I${DESTDIR}${X11INCDIR}/libdrm \
		-I${X11SRCDIR.drm} \
		-I${X11SRCDIR.drm}/include/drm \
		-I${X11SRCDIR.drm}/intel \
		-I${X11SRCDIR.drm}/amdgpu \
		-I${X11SRCDIR.drm}/radeon \
		-I${X11SRCDIR.Mesa}/src/loader \
		-I${X11SRCDIR.X11}/include \
		-I${X11SRCDIR.xorgproto}/include \
		-I${NETBSDSRCDIR}/external/apache2/llvm/dist/llvm/include \
		-I${LLVM_INCLUDE_OBJDIR} -I${NETBSDSRCDIR}/external/apache2/llvm/config
CPPFLAGS+=	-DVIDEO_CODEC_AV1DEC=1 -DVIDEO_CODEC_AV1ENC=1 \
		-DVIDEO_CODEC_VP9DEC=1 -DVIDEO_CODEC_MPEG12DEC=1 \
		-DVIDEO_CODEC_JPEGDEC=1 -DVIDEO_CODEC_VC1DEC=0 \
		-DVIDEO_CODEC_H264DEC=0 -DVIDEO_CODEC_H264ENC=0 \
		-DVIDEO_CODEC_H265DEC=0 -DVIDEO_CODEC_H265ENC=0

CFLAGS+=	-pthread
LDFLAGS+=	-pthread

LDFLAGS+=	-Wl,--build-id=sha1

# Reduce debugging for these extremely large objects.
.if ${MKDEBUG:Uno} != "no"
#CXXFLAGS+=	-g1
#CFLAGS+=	-g1
.endif

GALLIUM_SUBDIRS= \
	auxiliary \
	auxiliary/cso_cache \
	auxiliary/draw \
	auxiliary/driver_ddebug \
	auxiliary/driver_noop \
	auxiliary/driver_trace \
	auxiliary/hud \
	auxiliary/indices \
	auxiliary/nir \
	auxiliary/pipebuffer \
	auxiliary/pipe-loader \
	auxiliary/postprocess \
	auxiliary/renderonly \
	auxiliary/rtasm \
	auxiliary/tessellator \
	auxiliary/tgsi \
	auxiliary/translate \
	auxiliary/util \
	auxiliary/vl \
	drivers/softpipe \
	winsys/sw/dri \
	winsys/sw/kms-dri \
	winsys/sw/null \
	winsys/sw/wrapper \
	frontends/dri

GALLIUM_SUBDIRS_LLVMPIPE= \
	auxiliary/gallivm \
	drivers/llvmpipe

GALLIUM_SUBDIRS_ATI= \
	drivers/radeonsi \
	drivers/r300 \
	drivers/r300/compiler \
	drivers/r600 \
	drivers/r600/sfn \
	winsys/radeon/drm \
	winsys/amdgpu/drm

GALLIUM_SUBDIRS_I915= \
	drivers/i915 \
	winsys/i915/drm

GALLIUM_SUBDIRS_IRIS= \
	drivers/iris \
	winsys/iris/drm

GALLIUM_SUBDIRS_CROCUS= \
	drivers/crocus \
	winsys/crocus/drm

GALLIUM_SUBDIRS_NOUVEAU= \
	drivers/nouveau \
	drivers/nouveau/nv30 \
	drivers/nouveau/nv50 \
	drivers/nouveau/codegen \
	drivers/nouveau/nvc0 \
	winsys/nouveau/drm

BUILD_RADEON=0
BUILD_NOUVEAU=0
BUILD_LLVMPIPE=0
BUILD_I915=0
BUILD_IRIS=0
BUILD_CROCUS=0
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
BUILD_I915=1
#BUILD_IRIS=1
#BUILD_CROCUS=1
.endif
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || ${MACHINE} == "evbarm"
BUILD_NOUVEAU=1
. if ${MKLLVMRT} != "no"
BUILD_RADEON=1
. endif
.endif

.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE_ARCH} == "powerpc" || \
    ${MACHINE_ARCH} == "powerpc64" || \
    !empty(MACHINE_ARCH:Maarch64*) || \
    !empty(MACHINE_ARCH:Mmips*) || \
    !empty(MACHINE_ARCH:Mearm*) || \
    !empty(MACHINE_ARCH:Marm*)
. if ${MKLLVMRT} != "no"
BUILD_LLVMPIPE=1
. endif
.endif

DRIVERS=		kms_swrast swrast
.if ${BUILD_RADEON} == 1
DRIVERS+=		r600 r300 radeonsi
GALLIUM_SUBDIRS+=	${GALLIUM_SUBDIRS_ATI}
.endif
.if ${BUILD_NOUVEAU} == 1
DRIVERS+=		nouveau
GALLIUM_SUBDIRS+=	${GALLIUM_SUBDIRS_NOUVEAU}
.endif
.if ${BUILD_I915} == 1
GALLIUM_SUBDIRS+=	${GALLIUM_SUBDIRS_I915}
.endif
.if ${BUILD_IRIS} == 1
GALLIUM_SUBDIRS+=	${GALLIUM_SUBDIRS_IRIS}
.endif
.if ${BUILD_CROCUS} == 1
GALLIUM_SUBDIRS+=	${GALLIUM_SUBDIRS_CROCUS}
.endif
.if ${BUILD_LLVMPIPE} == 1
GALLIUM_SUBDIRS+=	${GALLIUM_SUBDIRS_LLVMPIPE}
.endif

GALLIUM_SOURCES.auxiliary/cso_cache= \
	cso_cache.c \
	cso_context.c \
	cso_hash.c

GALLIUM_SOURCES.auxiliary/draw= \
	draw_context.c \
	draw_fs.c \
	draw_gs.c \
	draw_mesh.c \
	draw_mesh_prim.c \
	draw_pipe_aaline.c \
	draw_pipe_aapoint.c \
	draw_pipe.c \
	draw_pipe_clip.c \
	draw_pipe_cull.c \
	draw_pipe_flatshade.c \
	draw_pipe_offset.c \
	draw_pipe_pstipple.c \
	draw_pipe_stipple.c \
	draw_pipe_twoside.c \
	draw_pipe_unfilled.c \
	draw_pipe_user_cull.c \
	draw_pipe_util.c \
	draw_pipe_validate.c \
	draw_pipe_vbuf.c \
	draw_pipe_wide_line.c \
	draw_pipe_wide_point.c \
	draw_prim_assembler.c \
	draw_pt.c \
	draw_pt_emit.c \
	draw_pt_fetch.c \
	draw_pt_fetch_shade_emit.c \
	draw_pt_fetch_shade_pipeline.c \
	draw_pt_mesh_pipeline.c \
	draw_pt_post_vs.c \
	draw_pt_so_emit.c \
	draw_pt_util.c \
	draw_pt_vsplit.c \
	draw_tess.c \
	draw_vertex.c \
	draw_vs.c \
	draw_vs_exec.c \
	draw_vs_variant.c

.if ${MKLLVMRT} != "no"
GALLIUM_SOURCES.auxiliary/draw+= \
	draw_llvm.c \
	draw_pt_fetch_shade_pipeline_llvm.c \
	draw_vs_llvm.c
.endif

GALLIUM_SOURCES.auxiliary/hud= \
	font.c \
	hud_context.c \
	hud_cpu.c \
	hud_nic.c \
	hud_cpufreq.c \
	hud_diskstat.c \
	hud_sensors_temp.c \
	hud_driver_query.c \
	hud_fps.c

GALLIUM_SOURCES.auxiliary/indices= \
	u_primconvert.c

GALLIUM_SOURCES.auxiliary/pipe-loader= \
	pipe_loader.c \
	pipe_loader_drm.c \
	pipe_loader_sw.c

.for _f in ${GALLIUM_SOURCES.auxiliary/pipe-loader}
CPPFLAGS.${_f} +=	-DHAVE_PIPE_LOADER_KMS=1 \
			-DHAVE_PIPE_LOADER_DRI=1 \
			-DHAVE_DRI=1 \
			-DHAVE_DRISW_KMS=1 \
			-DGALLIUM_STATIC_TARGETS=1 \
			-I${X11SRCDIR.Mesa}/src/gallium/winsys \
			-I${X11SRCDIR.Mesa}/src/loader \
			-I${X11SRCDIR.Mesa}/../src/util
.endfor

GALLIUM_SOURCES.auxiliary/pipebuffer= \
	pb_buffer_fenced.c \
	pb_bufmgr_cache.c \
	pb_bufmgr_debug.c \
	pb_bufmgr_mm.c \
	pb_bufmgr_slab.c \
	pb_cache.c \
	pb_validate.c

GALLIUM_SOURCES.auxiliary/postprocess= \
	pp_celshade.c \
	pp_colors.c \
	pp_init.c \
	pp_mlaa.c \
	pp_program.c \
	pp_run.c

GALLIUM_SOURCES.auxiliary/rtasm= \
	rtasm_execmem.c \
	rtasm_x86sse.c

GALLIUM_SOURCES.auxiliary/tessellator= \
	tessellator.cpp \
	p_tessellator.cpp

GALLIUM_SOURCES.auxiliary/tgsi= \
	tgsi_aa_point.c \
	tgsi_build.c \
	tgsi_dump.c \
	tgsi_dynamic_indexing.c \
	tgsi_exec.c \
	tgsi_from_mesa.c \
	tgsi_info.c \
	tgsi_iterate.c \
	tgsi_lowering.c \
	tgsi_parse.c \
	tgsi_point_sprite.c \
	tgsi_sanity.c \
	tgsi_scan.c \
	tgsi_strings.c \
	tgsi_text.c \
	tgsi_transform.c \
	tgsi_two_side.c \
	tgsi_ureg.c \
	tgsi_util.c \
	tgsi_vpos.c

GALLIUM_SOURCES.auxiliary/translate= \
	translate.c \
	translate_cache.c \
	translate_generic.c \
	translate_sse.c

GALLIUM_SOURCES.auxiliary/util= \
	u_async_debug.c \
	u_bitmask.c \
	u_blitter.c \
	u_debug_describe.c \
	u_debug_flush.c \
	u_debug_image.c \
	u_debug_refcnt.c \
	u_draw.c \
	u_draw_quad.c \
	u_driconf.c \
	u_dump_defines.c \
	u_dump_state.c \
	u_framebuffer.c \
	u_gen_mipmap.c \
	u_handle_table.c \
	u_helpers.c \
	u_index_modify.c \
	u_live_shader_cache.c \
	u_log.c \
	u_prim.c \
	u_prim_restart.c \
	u_pstipple.c \
	u_resource.c \
	u_sample_positions.c \
	u_sampler.c \
	u_screen.c \
	u_simple_shaders.c \
	u_split_draw.c \
	u_suballoc.c \
	u_surface.c \
	u_tests.c \
	u_texture.c \
	u_tile.c \
	u_transfer.c \
	u_transfer_helper.c \
	u_threaded_context.c \
	u_trace_gallium.c \
	u_upload_mgr.c \
	u_vbuf.c \
	u_vertex_state_cache.c

GALLIUM_SOURCES.auxiliary/nir= \
	tgsi_to_nir.c \
	nir_to_tgsi.c \
	nir_draw_helpers.c

.if ${MKLLVMRT} != "no"
GALLIUM_SOURCES.auxiliary/nir+=	nir_to_tgsi_info.c
.endif

GALLIUM_SOURCES.auxiliary/vl= \
	vl_codec.c \
	vl_compositor.c \
	vl_compositor_gfx.c \
	vl_compositor_cs.c \
	vl_csc.c \
	vl_deint_filter.c \
	vl_deint_filter_cs.c \
	vl_mpeg12_bitstream.c \
	vl_vertex_buffers.c \
	vl_video_buffer.c \
	vl_winsys_drm.c

# Generated
.PATH: ${X11SRCDIR.Mesa}/../src/gallium/auxiliary
GALLIUM_SOURCES.auxiliary= \
	draw_nir_lower_opcodes.c \
	tr_util.c \
	u_tracepoints.c \
	u_indices_gen.c \
	u_unfilled_gen.c

GALLIUM_SOURCES.auxiliary/renderonly= \
	renderonly.c

.if ${MKLLVMRT} != "no"
GALLIUM_SOURCES.auxiliary/gallivm= \
	lp_bld_arit.c \
	lp_bld_arit_overflow.c \
	lp_bld_assert.c \
	lp_bld_bitarit.c \
	lp_bld_const.c \
	lp_bld_conv.c \
	lp_bld_coro.c \
	lp_bld_flow.c \
	lp_bld_format_aos_array.c \
	lp_bld_format_aos.c \
	lp_bld_format_float.c \
	lp_bld_format.c \
	lp_bld_format_soa.c \
	lp_bld_format_s3tc.c \
	lp_bld_format_srgb.c \
	lp_bld_format_yuv.c \
	lp_bld_gather.c \
	lp_bld_init.c \
	lp_bld_init_common.c \
	lp_bld_intr.c \
	lp_bld_ir_common.c \
	lp_bld_jit_sample.c \
	lp_bld_jit_types.c \
	lp_bld_logic.c \
	lp_bld_misc.cpp \
	lp_bld_nir.c \
	lp_bld_nir_aos.c \
	lp_bld_nir_soa.c \
	lp_bld_pack.c \
	lp_bld_passmgr.c \
	lp_bld_printf.c \
	lp_bld_quad.c \
	lp_bld_sample_aos.c \
	lp_bld_sample.c \
	lp_bld_sample_soa.c \
	lp_bld_struct.c \
	lp_bld_swizzle.c \
	lp_bld_tgsi_action.c \
	lp_bld_tgsi.c \
	lp_bld_tgsi_info.c \
	lp_bld_tgsi_soa.c \
	lp_bld_type.c \
	lp_bld_debug.cpp
.endif

.for _f in ${GALLIUM_SOURCES.auxiliary/gallivm}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary/util
.endfor

CPPFLAGS.lp_texture.c += -I${X11SRCDIR.Mesa}/../src

GALLIUM_SOURCES.auxiliary/driver_ddebug= \
	dd_context.c \
	dd_draw.c \
	dd_screen.c

GALLIUM_SOURCES.auxiliary/driver_noop= \
	noop_pipe.c \
	noop_state.c

GALLIUM_SOURCES.auxiliary/driver_trace= \
	tr_context.c \
	tr_dump.c \
	tr_dump_state.c \
	tr_screen.c \
	tr_texture.c \
	tr_video.c

GALLIUM_SOURCES.drivers/i915= \
	i915_blit.c \
	i915_clear.c \
	i915_context.c \
	i915_debug.c \
	i915_debug_fp.c \
	i915_flush.c \
	i915_fpc_emit.c \
	i915_fpc_optimize.c \
	i915_fpc_translate.c \
	i915_prim_emit.c \
	i915_prim_vbuf.c \
	i915_query.c \
	i915_resource_buffer.c \
	i915_resource.c \
	i915_resource_texture.c \
	i915_screen.c \
	i915_state.c \
	i915_state_derived.c \
	i915_state_dynamic.c \
	i915_state_emit.c \
	i915_state_fpc.c \
	i915_state_immediate.c \
	i915_state_sampler.c \
	i915_state_static.c \
	i915_surface.c

GALLIUM_SOURCES.winsys/i915/drm= \
	i915_drm_batchbuffer.c \
	i915_drm_buffer.c \
	i915_drm_fence.c \
	i915_drm_winsys.c

GALLIUM_SOURCES.drivers/iris= \
	iris_batch.c \
	iris_binder.c \
	iris_blit.c \
	iris_border_color.c \
	iris_bufmgr.c \
	iris_clear.c \
	iris_context.c \
	iris_disk_cache.c \
	iris_draw.c \
	iris_fence.c \
	iris_fine_fence.c \
	iris_formats.c \
	iris_measure.c \
	iris_monitor.c \
	iris_perf.c \
	iris_performance_query.c \
	iris_pipe_control.c \
	iris_program.c \
	iris_program_cache.c \
	iris_resolve.c \
	iris_resource.c \
	iris_screen.c

GALLIUM_SOURCES.winsys/iris/drm= \
	iris_drm_winsys.c

IRIS_GENS_BLORP=	80 90 110 120 125

.for _gen in ${IRIS_GENS_BLORP}
BUILDSYMLINKS+=		${X11SRCDIR.Mesa}/src/gallium/drivers/iris/iris_blorp.c ${_gen}_iris_blorp.c
BUILDSYMLINKS+=		${X11SRCDIR.Mesa}/src/gallium/drivers/iris/iris_query.c ${_gen}_iris_query.c
BUILDSYMLINKS+=		${X11SRCDIR.Mesa}/src/gallium/drivers/iris/iris_state.c ${_gen}_iris_state.c
GALLIIUM_SOURCES.drivers/iris+=	${_gen}_iris_blorp.c ${_gen}_iris_query.c ${_gen}_iris_state.c

CPPFLAGS.${_gen}_iris_blorp.c+=		-DGFX_VERx10=${_gen}
CPPFLAGS.${_gen}_iris_query.c+=		-DGFX_VERx10=${_gen}
CPPFLAGS.${_gen}_iris_state.c+=		-DGFX_VERx10=${_gen}
.endfor

.for _f in ${GALLIUM_SOURCES.drivers/iris}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/intel
.endfor

GALLIUM_SOURCES.drivers/crocus= \
	crocus_batch.c \
	crocus_blit.c \
	crocus_blorp.c \
	crocus_blt.c \
	crocus_bufmgr.c \
	crocus_clear.c \
	crocus_context.c \
	crocus_disk_cache.c \
	crocus_draw.c \
	crocus_fence.c \
	crocus_fine_fence.c \
	crocus_formats.c \
	crocus_monitor.c \
	crocus_pipe_control.c \
	crocus_program.c \
	crocus_program_cache.c \
	crocus_query.c \
	crocus_resolve.c \
	crocus_resource.c \
	crocus_screen.c \
	crocus_state.c

GALLIUM_SOURCES.winsys/crocus/drm= \
	crocus_drm_winsys.c

CROCUS_GENS_BLORP=	40 45 50 60 70 75 80

.for _gen in ${CROCUS_GENS_BLORP}
BUILDSYMLINKS+=		${X11SRCDIR.Mesa}/src/gallium/drivers/crocus/crocus_blorp.c ${_gen}_crocus_blorp.c
BUILDSYMLINKS+=		${X11SRCDIR.Mesa}/src/gallium/drivers/crocus/crocus_query.c ${_gen}_crocus_query.c
BUILDSYMLINKS+=		${X11SRCDIR.Mesa}/src/gallium/drivers/crocus/crocus_state.c ${_gen}_crocus_state.c
BUILDSYMLINKS+=		${X11SRCDIR.Mesa}/src/gallium/drivers/crocus/crocus_blt.c ${_gen}_crocus_bltec
GALLIIUM_SOURCES.drivers/iris+=	${_gen}_crocus_blorp.c ${_gen}_crocus_query.c ${_gen}_crocus_state.c ${_gen}_crocus_blt.c 

CPPFLAGS.${_gen}_crocus_blorp.c+=	-DGFX_VERx10=${_gen}
CPPFLAGS.${_gen}_crocus_query.c+=	-DGFX_VERx10=${_gen}
CPPFLAGS.${_gen}_crocus_state.c+=	-DGFX_VERx10=${_gen}
CPPFLAGS.${_gen}_crocus_blt.c+=		-DGFX_VERx10=${_gen}
.endfor

.for _f in ${GALLIUM_SOURCES.drivers/crocus}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/intel
.endfor

GALLIUM_SOURCES.drivers/r600= \
	compute_memory_pool.c \
	eg_asm.c \
	eg_debug.c \
	evergreen_compute.c \
	evergreen_hw_context.c \
	evergreen_state.c \
	r600_asm.c \
	r600_blit.c \
	r600_dump.c \
	r600_hw_context.c \
	r600_isa.c \
	r600_pipe.c \
	r600_shader.c \
	r600_state.c \
	r600_state_common.c \
	r600_uvd.c \
	r700_asm.c \
	cayman_msaa.c \
	r600_buffer_common.c \
	r600_gpu_load.c \
	r600_perfcounter.c \
	r600_pipe_common.c \
	r600_query.c \
	r600_streamout.c \
	r600_test_dma.c \
	r600_texture.c \
	r600_viewport.c \
	radeon_uvd.c \
	r600_radeon_vce.c \
	radeon_video.c \
	sfn_nir_algebraic.c \
	r600_sfn.cpp

BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/gallium/drivers/r600/radeon_vce.c \
		r600_radeon_vce.c

GALLIUM_SOURCES.drivers/r600/sfn= \
	sfn_nir_vectorize_vs_inputs.c \
	sfn_alu_defines.cpp \
	sfn_alu_readport_validation.cpp \
	sfn_assembler.cpp \
	sfn_callstack.cpp \
	sfn_conditionaljumptracker.cpp \
	sfn_debug.cpp \
	sfn_instr.cpp \
	sfn_instr_alu.cpp \
	sfn_instr_alugroup.cpp \
	sfn_instr_controlflow.cpp \
	sfn_instr_export.cpp \
	sfn_instr_fetch.cpp \
	sfn_instr_mem.cpp \
	sfn_instr_lds.cpp \
	sfn_instr_tex.cpp \
	sfn_instrfactory.cpp \
	sfn_liverangeevaluator.cpp \
	sfn_liverangeevaluator_helpers.cpp \
	sfn_memorypool.cpp \
	sfn_nir.cpp \
	sfn_nir_legalize_image_load_store.cpp \
	sfn_nir_lower_64bit.cpp \
	sfn_nir_lower_alu.cpp \
	sfn_nir_lower_tex.cpp \
	sfn_nir_lower_fs_out_to_vector.cpp \
	sfn_nir_lower_tess_io.cpp \
	sfn_optimizer.cpp \
	sfn_peephole.cpp \
	sfn_ra.cpp \
	sfn_scheduler.cpp \
	sfn_shader.cpp \
	sfn_shader_cs.cpp \
	sfn_shader_fs.cpp \
	sfn_shader_gs.cpp \
	sfn_shader_tess.cpp \
	sfn_shader_vs.cpp \
	sfn_split_address_loads.cpp \
	sfn_valuefactory.cpp \
	sfn_virtualvalues.cpp

GALLIUM_SOURCES.drivers/r300= \
	r300_blit.c \
	r300_chipset.c \
	r300_context.c \
	r300_debug.c \
	r300_emit.c \
	r300_flush.c \
	r300_fs.c \
	r300_hyperz.c \
	r300_query.c \
	r300_render.c \
	r300_render_stencilref.c \
	r300_render_translate.c \
	r300_resource.c \
	r300_screen_buffer.c \
	r300_screen.c \
	r300_state.c \
	r300_state_derived.c \
	r300_texture.c \
	r300_texture_desc.c \
	r300_tgsi_to_rc.c \
	r300_transfer.c \
	r300_vs.c \
	r300_vs_draw.c \
	r300_nir_algebraic.c

CPPFLAGS.r300_state_derived.c = ${CC_WNO_STRINGOP_OVERREAD}

GALLIUM_SOURCES.drivers/r300/compiler= \
	memory_pool.c \
	nir_to_rc.c \
	r300_fragprog.c \
	r300_fragprog_emit.c \
	r300_fragprog_swizzle.c \
	r3xx_fragprog.c \
	r3xx_vertprog.c \
	r3xx_vertprog_dump.c \
	r500_fragprog.c \
	r500_fragprog_emit.c \
	r300_nir.c \
	r500_nir_lower_fcsel.c \
	radeon_code.c \
	radeon_compiler.c \
	radeon_compiler_util.c \
	radeon_dataflow.c \
	radeon_dataflow_deadcode.c \
	radeon_dataflow_swizzles.c \
	radeon_inline_literals.c \
	radeon_list.c \
	radeon_opcodes.c \
	radeon_optimize.c \
	radeon_pair_dead_sources.c \
	radeon_pair_regalloc.c \
	radeon_pair_schedule.c \
	radeon_pair_translate.c \
	radeon_program_alu.c \
	radeon_program.c \
	radeon_program_pair.c \
	radeon_program_print.c \
	radeon_program_tex.c \
	radeon_regalloc.c \
	radeon_remove_constants.c \
	radeon_variable.c \
	radeon_vert_fc.c

GALLIUM_SOURCES.drivers/radeonsi = \
	gfx11_query.c \
	si_barrier.c \
	si_buffer.c \
	si_clear.c \
	si_cp_dma.c \
	si_cp_utils.c \
	si_debug.c \
	si_descriptors.c \
	si_fence.c \
	si_get.c \
	si_gfx_cs.c \
	si_gpu_load.c \
	si_perfcounter.c \
	si_pipe.c \
	si_pm4.c \
	si_query.c \
	si_nir_clamp_shadow_comparison_value.c \
	si_nir_kill_outputs.c \
	si_nir_lower_abi.c \
	si_nir_lower_intrinsics_early.c \
	si_nir_lower_polygon_stipple.c \
	si_nir_lower_color_flatshade_twoside.c \
	si_nir_lower_resource.c \
	si_nir_lower_vs_inputs.c \
	si_nir_mark_divergent_texture_non_uniform.c \
	si_nir_optim.c \
	si_sdma_copy_image.c \
	si_state.c \
	si_state_binning.c \
	si_state_msaa.c \
	si_state_streamout.c \
	si_state_viewport.c \
	si_texture.c \
	si_utrace.c \
	si_video.c \
	si_vpe.c \
	si_video_dec.c \
	radeon_uvd_enc.c \
	radeon_vce.c \
	radeon_vcn.c \
	radeon_vcn_enc.c \
	radeon_vcn_enc_1_2.c \
	radeon_vcn_enc_2_0.c \
	radeon_vcn_enc_3_0.c \
	radeon_vcn_enc_4_0.c \
	radeon_vcn_enc_5_0.c \
	radeon_bitstream.c \
	si_blit.c \
	si_cp_reg_shadowing.c \
	si_compute_blit.c \
	si_compute.c \
	si_debug_gfx_compute.c \
	si_nir_mediump.c \
	si_mesh_shader.c \
	si_shader_aco.c \
	si_shader_args.c \
	si_shader_binary.c \
	si_shader_info.c \
	si_shader_nir.c \
	si_shader_variant_info.c \
	si_shader.c \
	si_shaderlib_nir.c \
	si_sqtt.c \
	si_test_dma_perf.c \
	si_test_image_copy_region.c \
	si_tracepoints.c \
	si_perfetto.cpp \
	si_state_shaders.cpp \
	si_state_draw_gfx6.cpp \
	si_state_draw_gfx7.cpp \
	si_state_draw_gfx8.cpp \
	si_state_draw_gfx9.cpp \
	si_state_draw_gfx10.cpp \
	si_state_draw_gfx103.cpp \
	si_state_draw_gfx11.cpp \
	si_state_draw_gfx115.cpp \
	si_state_draw_gfx12.cpp

.PATH:		${X11SRCDIR.Mesa}/src/gallium/drivers/radeonsi

.for _gen in 6 7 8 9 10 103 11 115 12
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/gallium/drivers/radeonsi/si_state_draw.cpp si_state_draw_gfx${_gen}.cpp
CPPFLAGS.si_state_draw_gfx${_gen}.cpp+=	-DGFX_VER=${_gen}
.endfor

.for _f in ${GALLIUM_SOURCES.drivers/r300} ${GALLIUM_SOURCES.drivers/r300/compiler}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/gallium/drivers/r300 \
			-I${X11SRCDIR.Mesa}/src/gallium/drivers/r300/compiler
.endfor

.for _f in ${GALLIUM_SOURCES.drivers/r600} ${GALLIUM_SOURCES.drivers/r600/sb} ${GALLIUM_SOURCES.drivers/radeonsi} ${GALLIUM_SOURCES.drivers/r600/sfn}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/gallium/drivers/r600
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/../src/gallium/drivers/r600
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/amd
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/amd/common
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/../src/amd/common
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/amd/common/nir
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/amd/compiler
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/../src/amd/compiler
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/amd/llvm
.endfor


GALLIUM_SOURCES.drivers/nouveau= \
	nouveau_buffer.c \
	nouveau_fence.c \
	nouveau_heap.c \
	nouveau_mm.c \
	nouveau_screen.c \
	nouveau_video.c \
	nouveau_vp3_video_bsp.c \
	nouveau_vp3_video.c \
	nouveau_vp3_video_vp.c

GALLIUM_SOURCES.drivers/nouveau/nv30= \
	nv30_clear.c \
	nv30_context.c \
	nv30_draw.c \
	nv30_format.c \
	nv30_fragprog.c \
	nv30_fragtex.c \
	nv30_miptree.c \
	nv30_push.c \
	nv30_query.c \
	nv30_resource.c \
	nv30_screen.c \
	nv30_state.c \
	nv30_state_validate.c \
	nv30_texture.c \
	nv30_transfer.c \
	nv30_vbo.c \
	nv30_vertprog.c \
	nv40_verttex.c \
	nvfx_fragprog.c \
	nvfx_vertprog.c

GALLIUM_SOURCES.drivers/nouveau/nv50= \
	nv50_compute.c \
	nv50_context.c \
	nv50_formats.c \
	nv50_miptree.c \
	nv50_program.c \
	nv50_push.c \
	nv50_query.c \
	nv50_query_hw.c \
	nv50_query_hw_metric.c \
	nv50_query_hw_sm.c \
	nv50_resource.c \
	nv50_screen.c \
	nv50_shader_state.c \
	nv50_state.c \
	nv50_state_validate.c \
	nv50_surface.c \
	nv50_tex.c \
	nv50_transfer.c \
	nv50_vbo.c \
	nv84_video_bsp.c \
	nv84_video.c \
	nv84_video_vp.c \
	nv98_video_bsp.c \
	nv98_video.c \
	nv98_video_ppp.c \
	nv98_video_vp.c

GALLIUM_SOURCES.drivers/nouveau/codegen= \
	nv50_ir.cpp \
	nv50_ir_bb.cpp \
	nv50_ir_build_util.cpp \
	nv50_ir_emit_nv50.cpp \
	nv50_ir_from_nir.cpp \
	nv50_ir_graph.cpp \
	nv50_ir_lowering_helper.cpp \
	nv50_ir_lowering_nv50.cpp \
	nv50_ir_peephole.cpp \
	nv50_ir_print.cpp \
	nv50_ir_ra.cpp \
	nv50_ir_serialize.cpp \
	nv50_ir_ssa.cpp \
	nv50_ir_target.cpp \
	nv50_ir_target_nv50.cpp \
	nv50_ir_util.cpp \
	nv50_ir_emit_gv100.cpp \
	nv50_ir_emit_gk110.cpp \
	nv50_ir_emit_gm107.cpp \
	nv50_ir_emit_nvc0.cpp \
	nv50_ir_lowering_gv100.cpp \
	nv50_ir_lowering_gm107.cpp \
	nv50_ir_lowering_nvc0.cpp \
	nv50_ir_target_gv100.cpp \
	nv50_ir_target_gm107.cpp \
	nv50_ir_target_nvc0.cpp

GALLIUM_SOURCES.drivers/nouveau/nvc0= \
	qmda0c0.c \
	qmdc0c0.c \
	qmdc3c0.c \
	nvc0_compute.c \
	nvc0_context.c \
	nvc0_formats.c \
	nvc0_miptree.c \
	nvc0_program.c \
	nvc0_query.c \
	nvc0_query_hw.c \
	nvc0_query_hw_metric.c \
	nvc0_query_hw_sm.c \
	nvc0_query_sw.c \
	nvc0_resource.c \
	nvc0_screen.c \
	nvc0_shader_state.c \
	nvc0_state.c \
	nvc0_state_validate.c \
	nvc0_surface.c \
	nvc0_tex.c \
	nvc0_transfer.c \
	nvc0_vbo.c \
	nvc0_vbo_translate.c \
	nvc0_video_bsp.c \
	nvc0_video.c \
	nvc0_video_ppp.c \
	nvc0_video_vp.c \
	nve4_compute.c

GALLIUM_SOURCES.winsys/nouveau/drm= \
	nouveau.c \
	nouveau_drm_winsys.c

.for _f in ${GALLIUM_SOURCES.drivers/nouveau} \
	   ${GALLIUM_SOURCES.drivers/nouveau/nv30} \
	   ${GALLIUM_SOURCES.drivers/nouveau/nv50} \
	   ${GALLIUM_SOURCES.drivers/nouveau/codegen} \
	   ${GALLIUM_SOURCES.drivers/nouveau/nvc0} \
	   ${GALLIUM_SOURCES.winsys/nouveau/drm}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/gallium/drivers/nouveau \
			-I${X11SRCDIR.Mesa}/src/gallium/drivers/nouveau/codegen \
			-I${X11SRCDIR.Mesa}/src/gallium/winsys/nouveau/drm \
			-I${X11SRCDIR.Mesa}/src/nouveau/drm \
			-I${X11SRCDIR.Mesa}/src/nouveau/headers \
			-I${X11SRCDIR.Mesa}/src/nouveau/headers/nvidia/classes \
			-I${X11SRCDIR.Mesa}/../src/nouveau/headers
.endfor

GALLIUM_SOURCES.winsys/radeon/drm= \
	radeon_drm_bo.c \
	radeon_drm_cs.c \
	radeon_drm_surface.c \
	radeon_drm_winsys.c \
	radeon_surface.c

GALLIUM_SOURCES.winsys/amdgpu/drm = \
	amdgpu_bo.c \
	amdgpu_surface.c \
	amdgpu_userq.c \
	amdgpu_winsys.c \
	amdgpu_cs.cpp
.for _f in ${GALLIUM_SOURCES.winsys/amdgpu/drm}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/amd
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/amd/common
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/../src/amd/common
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/amd/llvm
.endfor

GALLIUM_SOURCES.drivers/llvmpipe = \
	lp_bld_alpha.c \
	lp_bld_blend_aos.c \
	lp_bld_blend.c \
	lp_bld_blend_logicop.c \
	lp_bld_depth.c \
	lp_bld_interp.c \
	lp_clear.c \
	lp_context.c \
	lp_cs_tpool.c \
	lp_draw_arrays.c \
	lp_fence.c \
	lp_flush.c \
	lp_jit.c \
	lp_linear.c \
	lp_linear_fastpath.c \
	lp_linear_interp.c \
	lp_linear_sampler.c \
	lp_memory.c \
	lp_perf.c \
	lp_query.c \
	lp_rast.c \
	lp_rast_debug.c \
	lp_rast_linear.c \
	lp_rast_linear_fallback.c \
	lp_rast_rect.c \
	lp_rast_tri.c \
	lp_scene.c \
	lp_scene_queue.c \
	lp_screen.c \
	lp_setup.c \
	lp_setup_analysis.c \
	lp_setup_line.c \
	lp_setup_point.c \
	lp_setup_rect.c \
	lp_setup_tri.c \
	lp_setup_vbuf.c \
	lp_state_blend.c \
	lp_state_clip.c \
	lp_state_cs.c \
	lp_state_derived.c \
	lp_state_fs.c \
	lp_state_fs_analysis.c \
	lp_state_fs_fastpath.c \
	lp_state_fs_linear.c \
	lp_state_fs_linear_llvm.c \
	lp_state_gs.c \
	lp_state_rasterizer.c \
	lp_state_sampler.c \
	lp_state_setup.c \
	lp_state_so.c \
	lp_state_surface.c \
	lp_state_tess.c \
	lp_state_vertex.c \
	lp_state_vs.c \
	lp_surface.c \
	lp_tex_sample.c \
	lp_texture.c \
	lp_texture_handle.c

GALLIUM_SOURCES.drivers/softpipe= \
	sp_buffer.c \
	sp_clear.c \
	sp_context.c \
	sp_compute.c \
	sp_draw_arrays.c \
	sp_fence.c \
	sp_flush.c \
	sp_fs_exec.c \
	sp_image.c \
	sp_prim_vbuf.c \
	sp_quad_blend.c \
	sp_quad_depth_test.c \
	sp_quad_fs.c \
	sp_quad_pipe.c \
	sp_query.c \
	sp_screen.c \
	sp_setup.c \
	sp_state_blend.c \
	sp_state_clip.c \
	sp_state_derived.c \
	sp_state_image.c \
	sp_state_rasterizer.c \
	sp_state_sampler.c \
	sp_state_shader.c \
	sp_state_so.c \
	sp_state_surface.c \
	sp_state_vertex.c \
	sp_surface.c \
	sp_tex_sample.c \
	sp_tex_tile_cache.c \
	sp_texture.c \
	sp_tile_cache.c

GALLIUM_SOURCES.winsys/sw/null = \
	null_sw_winsys.c

GALLIUM_SOURCES.winsys/sw/wrapper = \
	wrapper_sw_winsys.c

GALLIUM_SOURCES.winsys/sw/dri = \
	dri_sw_winsys.c

GALLIUM_SOURCES.winsys/sw/kms-dri = \
	kms_dri_sw_winsys.c

GALLIUM_SOURCES.frontends/dri = \
	dri_context.c \
	dri_drawable.c \
	dri_helpers.c \
	dri_query_renderer.c \
	dri_screen.c \
	dri_util.c \
	drisw.c \
	dri2.c \
	kopper_stubs.c \
	loader_dri3_helper.c

# Sources kept in standalone Meson helper libraries upstream.
SRCS+=		intel_gem.c common_intel_engine.c i915_intel_gem.c \
		i915_intel_engine.c xe_intel_gem.c xe_intel_engine.c \
		xe_intel_device_query.c nv_push.c
.PATH:		${X11SRCDIR.Mesa}/src/intel/common
.PATH:		${X11SRCDIR.Mesa}/src/nouveau/headers
.PATH:		${X11SRCDIR.Mesa}/../src/nouveau/headers
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/common/i915/intel_gem.c i915_intel_gem.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/common/i915/intel_engine.c i915_intel_engine.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/common/xe/intel_gem.c xe_intel_gem.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/common/xe/intel_engine.c xe_intel_engine.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/common/xe/intel_device_query.c xe_intel_device_query.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/intel/common/intel_engine.c common_intel_engine.c
.for _intel_src in intel_gem.c common_intel_engine.c i915_intel_gem.c \
		i915_intel_engine.c xe_intel_gem.c xe_intel_engine.c \
		xe_intel_device_query.c
CPPFLAGS.${_intel_src}+=	-I${X11SRCDIR.Mesa}/src/intel \
				-I${X11SRCDIR.Mesa}/src/intel/common
.endfor
CPPFLAGS.nv_push.c+=	-I${X11SRCDIR.Mesa}/src/nouveau/headers \
			-I${X11SRCDIR.Mesa}/src/nouveau/headers/nvidia/classes \
			-I${X11SRCDIR.Mesa}/src/nouveau/headers/nvidia \
			-I${X11SRCDIR.Mesa}/../src/nouveau/headers

NV_PUSH_CLASSES=	85b5 902d 9039 906f 9097 90b5 90c0 91c0 9297 \
		a040 a0b5 a06f a097 a140 a16f a1c0 a197 a26f a297 \
		b06f b097 b0b5 b0c0 b197 b1c0 c06f c097 c197 c1b5 \
		c1c0 a0c0 c0b5 c0c0 c36f c397 c3b5 c3c0 c46f c56f \
		c597 c5b0 c5b5 c5c0 c697 c6b5 c6c0 c797 c7b0 c7b5 \
		c7c0 c86f c8b5 c96f c997 c9b0 c9b5 c9c0 ca6f cab5 \
		cb97 cbc0 cd97 cdc0 cec0 ce97
.for _cl in ${NV_PUSH_CLASSES}
SRCS+=		nv_push_cl${_cl}.c
CPPFLAGS.nv_push_cl${_cl}.c+=	-I${X11SRCDIR.Mesa}/src/nouveau/headers \
					-I${X11SRCDIR.Mesa}/src/nouveau/headers/nvidia/classes \
					-I${X11SRCDIR.Mesa}/../src/nouveau/headers
.endfor

# missing
CPPFLAGS+=	\
	-DGALLIUM_SOFTPIPE \
	-DHAVE_R300 -DHAVE_R600 -DHAVE_RADEONSI -DHAVE_GFX_COMPUTE \
	-DADDR_FASTCALL= \
	-DGALLIUM_STATIC_TARGETS=1 \
	-DMESA_EGL_NO_X11_HEADERS

.for _d in ${GALLIUM_SUBDIRS}
SRCS+=	${GALLIUM_SOURCES.${_d}}
.PATH: ${X11SRCDIR.Mesa}/src/gallium/${_d}
.PATH: ${X11SRCDIR.Mesa}/../src/gallium/${_d}
. for _s in ${GALLIUM_SOURCES.${_d}}
CPPFLAGS.${_s}+= -I${X11SRCDIR.Mesa}/src/gallium/${_d}
CPPFLAGS.${_s}+= -I${X11SRCDIR.Mesa}/../src/gallium/${_d}
. endfor
.endfor

#. if defined(${GALLIUM_SOURCES.${_d}})
#SRCS+=	${GALLIUM_SOURCES.${_d}}
#. endif

.include "../libloader.mk"

.if ${BUILD_RADEON} == 1
CFLAGS+=	-pthread
LDFLAGS+=	-pthread
.endif

LIBDPLIBS+=	X11-xcb		${.CURDIR}/../libX11/libX11-xcb
LIBDPLIBS+=	xcb-dri2	${.CURDIR}/../libxcb/dri2
LIBDPLIBS+=	xcb-dri3	${.CURDIR}/../libxcb/dri3
LIBDPLIBS+=	xcb-present	${.CURDIR}/../libxcb/present
LIBDPLIBS+=	xcb-randr	${.CURDIR}/../libxcb/randr
LIBDPLIBS+=	xcb-sync	${.CURDIR}/../libxcb/sync
LIBDPLIBS+=	xcb-xfixes	${.CURDIR}/../libxcb/xfixes
LIBDPLIBS+=	xcb		${.CURDIR}/../libxcb/libxcb
LIBDPLIBS+=	xshmfence	${.CURDIR}/../libxshmfence
LIBDPLIBS+= 	drm		${.CURDIR}/../libdrm
.if ${BUILD_I915} == 1 || ${BUILD_IRIS} == 1 || ${BUILD_CROCUS} == 1
LIBDPLIBS+= 	drm_intel	${.CURDIR}/../libdrm_intel
.endif
.if ${BUILD_RADEON} == 1
LIBDPLIBS+= 	drm_radeon	${.CURDIR}/../libdrm_radeon
LIBDPLIBS+= 	drm_amdgpu	${.CURDIR}/../libdrm_amdgpu
.endif	# ${BUILD_RADEON} == 1
# Gallium uses internal GLAPI entry points which are intentionally hidden from
# libglapi.so.  Upstream links the static GLAPI archive into the DRI target.
DPADD+=		${.OBJDIR}/../libglapi${OLD_SUFFIX}/libglapi_pic.a
LDADD+=		${.OBJDIR}/../libglapi${OLD_SUFFIX}/libglapi_pic.a
LIBDPLIBS+=	expat		${.CURDIR}/../../../../../external/mit/expat/lib/libexpat
LIBDPLIBS+=	terminfo	${.CURDIR}/../../../../../lib/libterminfo
LIBDPLIBS+=	execinfo	${.CURDIR}/../../../../../lib/libexecinfo

LIBDPLIBS+=	elf		${NETBSDSRCDIR}/external/bsd/elftoolchain/lib/libelf
LIBDPLIBS+=	z		${.CURDIR}/../../../../../lib/libz
LIBDPLIBS+=	m		${.CURDIR}/../../../../../lib/libm

# gallium drivers requiring LLVM
.if ${BUILD_LLVMPIPE} == 1 || ${BUILD_RADEON} == 1

# LLVM support is provided by the canonical target shared library.
LLVM_LIBS=	Support

.include "${NETBSDSRCDIR}/external/apache2/llvm/link.mk"

.endif  # ${BUILD_LLVM_PIPE} == 1 || ${BUILD_RADEON} == 1

LDFLAGS+=	-Wl,--version-script=${X11SRCDIR.Mesa}/../src/gallium/targets/dri/dri.sym
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || ${MACHINE_CPU} == "aarch64"
LDFLAGS+=	-Wl,-z,defs
.endif

##  build mesagallium parts
MESA_SRC_MODULES=  main math vbo state_tracker program asm_s
.if ${BUILD_RADEON} == 1
MESA_SRC_MODULES+= amd
.endif

.include "../libmesa.mk"
.include "../libglsl.mk"

# Needs 64bit atomics
CPPFLAGS.i386+= -march=i586
CPPFLAGS+=	${CPPFLAGS.${XORG_MACHINE_ARCH:U${MACHINE_ARCH}}}

CPPFLAGS.r600_pipe_common.c+=	-DMESA_LLVM_VERSION_STRING=\"${LLVM_VERSION}\"
CPPFLAGS.si_get.c+=	-DMESA_LLVM_VERSION_STRING=\"${LLVM_VERSION}\"
CPPFLAGS.lp_screen.c+=	-DMESA_LLVM_VERSION_STRING=\"${LLVM_VERSION}\"
CPPFLAGS.lp_bld_intr.c+=	-DMESA_LLVM_VERSION_STRING=\"${LLVM_VERSION}\"
CPPFLAGS.lp_bld_type.c+=	-DMESA_LLVM_VERSION_STRING=\"${LLVM_VERSION}\"

.include "../driver.mk"


.PATH: ${X11SRCDIR.Mesa}/src/gallium/targets/dri
SRCS+=	dri_target.c

CPPFLAGS+= \
	-I${X11SRCDIR.Mesa}/src/gallium/include \
	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary \
	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary/util \
	-I${X11SRCDIR.Mesa}/../src/gallium/auxiliary \
	-I${X11SRCDIR.Mesa}/src/gallium/drivers \
	-I${X11SRCDIR.Mesa}/../src/mesa/drivers/dri/common

CPPFLAGS.dri_target.c += \
	-DDRI_TARGET \
	-DGALLIUM_NOOP \
	-DGALLIUM_RBUG \
	-DGALLIUM_TRACE \
	-DGALLIUM_SOFTPIPE \
	-I${X11SRCDIR.Mesa}/src/gallium/frontends/dri \
	-I${X11SRCDIR.Mesa}/src/loader \
	-I${X11SRCDIR.Mesa}/../src/util \
	-I${X11SRCDIR.Mesa}/../src/gallium/drivers \
	-I${X11SRCDIR.Mesa}/src/gallium/winsys

.if ${BUILD_LLVMPIPE} == 1
CPPFLAGS.dri_target.c += \
	-DGALLIUM_LLVMPIPE
.endif  #${BUILD_LLVMPIPE} == 1

.if ${BUILD_I915} == 1
CPPFLAGS.dri_target.c += \
	-DGALLIUM_I915 \
	-DGALLIUM_ILO
.endif

.if ${BUILD_IRIS} == 1
CPPFLAGS.dri_target.c += \
	-DGALLIUM_IRIS
.endif

.if ${BUILD_CROCUS} == 1
CPPFLAGS.dri_target.c += \
	-DGALLIUM_CROCUS
.endif

.if ${BUILD_RADEON} == 1
CPPFLAGS.dri_target.c += \
	-DGALLIUM_R200 \
	-DGALLIUM_R300 \
	-DGALLIUM_R600 \
	-DGALLIUM_RADEONSI
.endif	# ${BUILD_RADEON} == 1

.if ${BUILD_NOUVEAU} == 1
CPPFLAGS.dri_target.c += \
	-DGALLIUM_NOUVEAU
.endif	# ${BUILD_NOUVEAU} == 1

CWARNFLAGS.clang+=	-Wno-error=constant-conversion \
			-Wno-error=tautological-constant-out-of-range-compare \
			-Wno-error=pointer-sign \
			-Wno-error=switch \
			-Wno-error=absolute-value \
			-Wno-error=tautological-compare \
			-Wno-error=static-in-inline \
			-Wno-error=logical-not-parentheses \
			-Wno-error=constant-logical-operand \
			-Wno-error=unknown-warning-option \
			-Wno-error=typedef-redefinition \
			-Wno-error=enum-conversion # https://bugs.freedesktop.org/show_bug.cgi?id=109761

CWARNFLAGS+=		-Wno-error=stack-protector

.include <bsd.x11.mk>

SYMLINKS+= ../../libgallium.so.${SHLIB_MAJOR} ${DRIDIR}/gallium_dri.so
SYMLINKS+= ../../libgallium.so.${SHLIB_MAJOR} ${DRIDIR}/gallium_dri.so.${SHLIB_MAJOR}
.if ${MKDEBUG} != "no"
SYMLINKS+= ../../libgallium.so.${SHLIB_MAJOR}.${SHLIB_MINOR}.debug \
	${DRIDEBUGDIR}/gallium_dri.so.${SHLIB_MAJOR}.debug
.endif

.for _d in ${DRIVERS}
SYMLINKS+= gallium_dri.so ${DRIDIR}/${_d}_dri.so
SYMLINKS+= gallium_dri.so.${SHLIB_MAJOR} ${DRIDIR}/${_d}_dri.so.${SHLIB_MAJOR} 
.if ${MKDEBUG} != "no"
SYMLINKS+= gallium_dri.so.${SHLIB_MAJOR}.debug ${DRIDEBUGDIR}/${_d}_dri.so.${SHLIB_MAJOR}.debug
.endif
.endfor

.if ${MACHINE_ARCH} == "sparc" || ${COMMON_MACHINE_ARCH:U} == "sparc"
COPTS+=	${${ACTIVE_CC} == "clang":? -Wa,-Av8plus  :}
.endif

COPTS.u_atomic.c+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 10:? -Wno-builtin-declaration-mismatch :}

.if ${MACHINE} == "vax" && ${HAVE_GCC:U0} >= 12
# in extract_constrain_insn, at recog.cc:2698
COPTS.sp_quad_depth_test.c+=	-O0
# in find_reloads_toplev, at reload.cc:4815
COPTS.u_format_table.c+=	-O0
.endif

.if ${MACHINE} == "vax" && ${HAVE_GCC:U0} == 10
# in fixup_reorder_chain, at cfgrtl.c:3985
COPTS.nir_builtin_builder.c+=	-O0
# in fixup_reorder_chain, at cfgrtl.c:3985
COPTS.nir_lower_int64.c+=	-O0
.endif

.if ${MACHINE_CPU} == "m68k" && ${HAVE_GCC:U0} == 12
COPTS.vl_bicubic_filter.c+=	-O1
.endif

.if ${MACHINE_CPU} == "m68k" && ${HAVE_GCC:U0} >= 12
COPTS.format_utils.c+=	-Os
COPTS.glsl_lexer.cpp+=	-O1 -fno-jump-tables
.endif
.if ${MACHINE_CPU} == "m68k" && ${HAVE_GCC:U0} >= 14
COPTS.glsl_parser.cpp+=	-Os
.endif

.include <bsd.lib.mk>
# Don't regenerate c files
.y.c:
