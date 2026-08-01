#	$NetBSD: libmesa.mk,v 1.16 2026/02/22 08:36:02 mrg Exp $
#
# Consumer of this Makefile should set MESA_SRC_MODULES.

CPPFLAGS.ac_surface.c+=	${${ACTIVE_CC} == "clang":? -Wno-error=enum-conversion :}

# The source file lists derived from src/mesa/Makefile.sources.
# Please keep the organization in line with those files.

# Main sources
PATHS.main=	mesa/main mesa/glapi/glapi
PATHS.main+=	../../src/mesa ../../src/mesa/main ../../src/mesa/glapi/glapi/gen
INCLUDES.main=	glsl mesa/main ../../src/compiler/nir ../../src/mesa
INCLUDES.main+= gallium/auxiliary ../../src
SRCS.main= \
	format_fallback.c \
	accum.c \
	api_arrayelt.c \
	arbprogram.c \
	arrayobj.c \
	atifragshader.c \
	attrib.c \
	barrier.c \
	bbox.c \
	blend.c \
	blit.c \
	bufferobj.c \
	buffers.c \
	clear.c \
	clip.c \
	compute.c \
	condrender.c \
	conservativeraster.c \
	context.c \
	copyimage.c \
	MESAdebug.c \
	debug_output.c \
	depth.c \
	dlist.c \
	draw.c \
	draw_validate.c \
	drawpix.c \
	drawtex.c \
	enable.c \
	errors.c \
	es1_conversion.c \
	MESAeval.c \
	extensions.c \
	extensions_table.c \
	externalobjects.c \
	fbobject.c \
	feedback.c \
	ff_fragment_shader.c \
	ffvertex_prog.c \
	fog.c \
	format_utils.c \
	formatquery.c \
	formats.c \
	framebuffer.c \
	genmipmap.c \
	get.c \
	getstring.c \
	glformats.c \
	glspirv.c \
	glthread.c \
	glthread_bufferobj.c \
	glthread_draw.c \
	glthread_draw_unroll.c \
	glthread_get.c \
	glthread_list.c \
	glthread_pixels.c \
	glthread_shaderobj.c \
	glthread_varray.c \
	hash.c \
	hint.c \
	image.c \
	light.c \
	lines.c \
	matrix.c \
	mesh_shader.c \
	mipmap.c \
	multisample.c \
	objectlabel.c \
	pack.c \
	pbo.c \
	performance_monitor.c \
	performance_query.c \
	pipelineobj.c \
	MESApixel.c \
	MESApixelstore.c \
	pixeltransfer.c \
	points.c \
	polygon.c \
	program_binary.c \
	program_resource.c \
	querymatrix.c \
	queryobj.c \
	rastpos.c \
	readpix.c \
	renderbuffer.c \
	robustness.c \
	samplerobj.c \
	scissor.c \
	shaderapi.c \
	shaderimage.c \
	shaderobj.c \
	shared.c \
	spirv_capabilities.c \
	spirv_extensions.c \
	state.c \
	stencil.c \
	syncobj.c \
	texcompress.c \
	texcompress_bptc.c \
	texcompress_cpal.c \
	texcompress_etc.c \
	texcompress_fxt1.c \
	texcompress_rgtc.c \
	texcompress_s3tc.c \
	texenv.c \
	texgen.c \
	texgetimage.c \
	teximage.c \
	texobj.c \
	texparam.c \
	texstate.c \
	texstorage.c \
	texstore.c \
	texturebindless.c \
	textureview.c \
	transformfeedback.c \
	uniforms.c \
	varray.c \
	version.c \
	viewport.c \
	shader_query.cpp \
	uniform_query.cpp \
	api_exec_init.c \
	enums.c \
	unmarshal_table.c \
	marshal_generated0.c \
	marshal_generated1.c \
	marshal_generated2.c \
	marshal_generated3.c \
	marshal_generated4.c \
	marshal_generated5.c \
	marshal_generated6.c \
	marshal_generated7.c

# AMD common, compiler, address and video-processing code
PATHS.amd= \
	amd/addrlib/src \
	amd/addrlib/src/core \
	amd/addrlib/src/gfx9 \
	amd/addrlib/src/gfx10 \
	amd/addrlib/src/gfx11 \
	amd/addrlib/src/gfx12 \
	amd/addrlib/src/r800 \
	amd/common \
	amd/common/nir \
	../../src/amd/common \
	amd/compiler/instruction_selection \
	amd/compiler \
	../../src/amd/compiler \
	amd/vpelib/src/core \
	amd/vpelib/src/utils \
	amd/vpelib/src/chip/vpe10 \
	amd/vpelib/src/chip/vpe11 \
	amd/gmlib \
	amd/gmlib/gm \
	amd/gmlib/ToneMapGenerator/src/src \
	amd/lanczoslib \
	amd/lanczoslib/lanczosFilter/src

INCLUDES.amd= \
	amd \
	../../src/amd \
	amd/common \
	../../src/amd/common \
	amd/common/nir \
	amd/addrlib \
	amd/addrlib/inc \
	amd/addrlib/src \
	amd/addrlib/src/core \
	amd/addrlib/src/r800 \
	amd/addrlib/src/chip/r800 \
	amd/addrlib/src/gfx9 \
	amd/addrlib/src/chip/gfx9 \
	amd/addrlib/src/gfx10 \
	amd/addrlib/src/chip/gfx10 \
	amd/addrlib/src/gfx11 \
	amd/addrlib/src/chip/gfx11 \
	amd/addrlib/src/gfx12 \
	amd/addrlib/src/chip/gfx12 \
	amd/compiler \
	../../src/amd/compiler \
	amd/compiler/instruction_selection \
	amd/vpelib/inc \
	amd/vpelib/src \
	amd/vpelib/src/core/inc \
	amd/vpelib/src/chip \
	amd/vpelib/src/utils/inc \
	amd/vpelib/src/chip/vpe10/inc \
	amd/vpelib/src/chip/vpe11/inc \
	amd/gmlib \
	amd/gmlib/gm \
	amd/gmlib/ToneMapGenerator/inc \
	amd/gmlib/ToneMapGenerator/src/inc \
	amd/gmlib/ToneMapGenerator/src/src \
	amd/lanczoslib \
	amd/lanczoslib/lanczosFilter/src \
	compiler \
	gallium/include \
	gallium/auxiliary \
	../src/util

SRCS.amd= \
	addrinterface.cpp \
	addrelemlib.cpp \
	addrlib.cpp \
	addrlib1.cpp \
	addrlib2.cpp \
	addrlib3.cpp \
	addrobject.cpp \
	addrswizzler.cpp \
	coord.cpp \
	gfx9addrlib.cpp \
	gfx10addrlib.cpp \
	gfx11addrlib.cpp \
	gfx12addrlib.cpp \
	ciaddrlib.cpp \
	egbaddrlib.cpp \
	siaddrlib.cpp \
	ac_binary.c \
	ac_cmdbuf.c \
	ac_cmdbuf_cp.c \
	ac_cmdbuf_sdma.c \
	ac_shader_args.c \
	ac_shader_util.c \
	ac_guardband.c \
	ac_gather_context_rolls.c \
	ac_gpu_info.c \
	ac_surface.c \
	ac_debug.c \
	ac_descriptors.c \
	ac_formats.c \
	ac_shadowed_regs.c \
	ac_spm.c \
	ac_sqtt.c \
	ac_rgp.c \
	ac_msgpack.c \
	amd_family.c \
	ac_parse_ib.c \
	ac_perfcounter.c \
	ac_perfcounter_gfx10.c \
	ac_perfcounter_gfx103.c \
	ac_perfcounter_gfx11.c \
	ac_perfcounter_gfx12.c \
	ac_pm4.c \
	ac_vcn_dec.c \
	ac_vcn_enc.c \
	ac_uvd_dec.c \
	ac_video_dec.c \
	ac_nir.c \
	ac_nir_opt_outputs.c \
	ac_nir_cull.c \
	ac_nir_create_gs_copy_shader.c \
	ac_nir_lower_esgs_io_to_mem.c \
	ac_nir_lower_global_access.c \
	ac_nir_lower_image_opcodes_cdna.c \
	ac_nir_lower_image_tex.c \
	ac_nir_lower_intrinsics_to_args.c \
	ac_nir_lower_legacy_gs.c \
	ac_nir_lower_legacy_vs.c \
	ac_nir_lower_mem_access_bit_sizes.c \
	ac_nir_lower_resinfo.c \
	ac_nir_lower_taskmesh_io_to_mem.c \
	ac_nir_lower_tess_io_to_mem.c \
	ac_nir_lower_ngg.c \
	ac_nir_lower_ngg_gs.c \
	ac_nir_lower_ngg_mesh.c \
	ac_nir_lower_ps_early.c \
	ac_nir_lower_ps_late.c \
	ac_nir_meta_cs_blit.c \
	ac_nir_meta_cs_clear_copy_buffer.c \
	ac_nir_meta_ps_resolve.c \
	ac_nir_opt_flip_if_for_mem_loads.c \
	ac_nir_opt_shared_append.c \
	ac_nir_prerast_utils.c \
	ac_nir_surface.c \
	ac_linux_drm.c \
	ac_rtld.c \
	ac_rgp_elf_object_pack.c \
	amd_cp_print_packet_gfx11.c \
	amd_cp_print_packet_gfx12.c \
	gfx10_format_table.c \
	aco_isel_cfg.cpp \
	aco_isel_helpers.cpp \
	aco_isel_setup.cpp \
	aco_select_nir_alu.cpp \
	aco_select_nir_intrinsics.cpp \
	aco_select_nir.cpp \
	aco_select_ps_epilog.cpp \
	aco_select_ps_prolog.cpp \
	aco_select_trap_handler.cpp \
	aco_select_vs_prolog.cpp \
	aco_dead_code_analysis.cpp \
	aco_dominance.cpp \
	aco_interface.cpp \
	aco_ir.cpp \
	aco_assembler.cpp \
	aco_form_hard_clauses.cpp \
	aco_insert_delay_alu.cpp \
	aco_insert_exec_mask.cpp \
	aco_insert_fp_mode.cpp \
	aco_insert_NOPs.cpp \
	aco_insert_waitcnt.cpp \
	aco_reduce_assign.cpp \
	aco_register_allocation.cpp \
	aco_live_var_analysis.cpp \
	aco_lower_branches.cpp \
	aco_lower_phis.cpp \
	aco_lower_subdword.cpp \
	aco_lower_to_cssa.cpp \
	aco_lower_to_hw_instr.cpp \
	aco_optimizer.cpp \
	aco_optimizer_postRA.cpp \
	aco_opt_value_numbering.cpp \
	aco_print_asm.cpp \
	aco_print_ir.cpp \
	aco_reindex_ssa.cpp \
	aco_repair_ssa.cpp \
	aco_scheduler.cpp \
	aco_scheduler_ilp.cpp \
	aco_spill.cpp \
	aco_spill_preserved.cpp \
	aco_ssa_elimination.cpp \
	aco_statistics.cpp \
	aco_validate.cpp \
	aco_opcodes.cpp \
	color_gamma.c \
	color_bg.c \
	vpe_scl_filters.c \
	background.c \
	vpe_visual_confirm.c \
	mpc.c \
	config_writer.c \
	color_gamut.c \
	vpelib.c \
	3dlut_builder.c \
	geometric_scaling.c \
	color_test_values.c \
	resource.c \
	color_table.c \
	color.c \
	color_cs.c \
	common.c \
	shaper_builder.c \
	custom_fp16.c \
	custom_float.c \
	conversion.c \
	fixpt31_32.c \
	vector.c \
	vpe10_plane_desc_writer.c \
	vpe10_vpe_desc_writer.c \
	vpe10_cm_common.c \
	vpe10_dpp.c \
	vpe10_resource.c \
	vpe10_mpc.c \
	vpe10_cmd_builder.c \
	vpe10_dpp_dscl.c \
	vpe10_dpp_cm.c \
	vpe10_opp.c \
	vpe10_background.c \
	vpe10_cdc_fe.c \
	vpe10_cdc_be.c \
	vpe10_vpec.c \
	vpe10_config_writer.c \
	vpe11_cmd_builder.c \
	vpe11_resource.c \
	vpe11_vpe_desc_writer.c \
	tonemap_adaptor.c \
	csc_api_funcs.c \
	csc_funcs.c \
	cs_funcs.c \
	cvd_api_funcs.c \
	cvd_funcs.c \
	gm_api_funcs.c \
	gm_funcs.c \
	mat_funcs.c \
	AGMGenerator.c \
	ToneMapGenerator.c \
	lanczos_adaptor.cpp \
	lanczosFilterGenerator.cpp

.for _f in tonemap_adaptor.c csc_api_funcs.c csc_funcs.c cs_funcs.c \
	    cvd_api_funcs.c cvd_funcs.c gm_api_funcs.c gm_funcs.c mat_funcs.c \
	    AGMGenerator.c ToneMapGenerator.c
CPPFLAGS.${_f}+=	-DGM_SIM
.endfor

# XXX  avoid source name clashes with glx
.PATH:		${X11SRCDIR.Mesa}/src/mesa/main
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/mesa/main/pixel.c MESApixel.c \
		${X11SRCDIR.Mesa}/src/mesa/main/pixelstore.c MESApixelstore.c \
		${X11SRCDIR.Mesa}/src/mesa/main/eval.c MESAeval.c \
		${X11SRCDIR.Mesa}/src/mesa/main/debug.c MESAdebug.c

# Math sources
PATHS.math=	mesa/math
SRCS.math= \
	m_eval.c \
	m_matrix.c

PATHS.math_xform=	mesa/math
SRCS.math_xform= \
	m_xform.c


# VBO sources
PATHS.vbo=	mesa/vbo
INCLUDES.vbo=	gallium/auxiliary
SRCS.vbo= \
	vbo_context.c \
	vbo_exec.c \
	vbo_exec_api.c \
	vbo_exec_draw.c \
	vbo_exec_eval.c \
	vbo_minmax_index.c \
	vbo_noop.c \
	vbo_save.c \
	vbo_save_api.c \
	vbo_save_draw.c \
	vbo_save_loopback.c

# TNL sources
PATHS.tnl=	mesa/tnl
SRCS.tnl= \
	t_context.c \
	t_draw.c \
	t_pipeline.c \
	t_rebase.c \
	t_split.c \
	t_split_copy.c \
	t_split_inplace.c \
	t_vb_fog.c \
	t_vb_light.c \
	t_vb_normals.c \
	t_vb_points.c \
	t_vb_program.c \
	t_vb_render.c \
	t_vb_texgen.c \
	t_vb_texmat.c \
	t_vb_vertex.c \
	t_vertex.c \
	t_vertex_generic.c \
	t_vertex_sse.c \
	t_vp_build.c

# Software raster sources
PATHS.swrast=		mesa/swrast
INCLUDES.swrast=	mesa/main
SRCS.swrast= \
	s_aaline.c \
	s_aatriangle.c \
	s_alpha.c \
	s_atifragshader.c \
	s_bitmap.c \
	s_blend.c \
	s_blit.c \
	s_clear.c \
	s_context.c \
	s_copypix.c \
	s_depth.c \
	s_drawpix.c \
	s_feedback.c \
	s_fog.c \
	s_fragprog.c \
	s_lines.c \
	s_logic.c \
	s_masking.c \
	s_points.c \
	s_renderbuffer.c \
	s_span.c \
	s_stencil.c \
	s_texcombine.c \
	s_texfetch.c \
	s_texfilter.c \
	s_texrender.c \
	s_texture.c \
	s_triangle.c \
	s_zoom.c

# swrast_setup
PATHS.ss=	mesa/swrast_setup
SRCS.ss= \
	ss_context.c \
	ss_triangle.c 


# Common driver sources
PATHS.common=	mesa/drivers/common
SRCS.common= \
	driverfuncs.c   \
	meta_blit.c     \
	meta_generate_mipmap.c  \
	meta.c

# ASM C driver sources
PATHS.asm_c=	mesa/x86 mesa/x86/rtasm mesa/sparc mesa/x86-64
SRCS.asm_c= \
	common_x86.c \
	x86_xform.c \
	3dnow.c \
	sse.c \
	x86sse.c \
	sparc.c \
	x86-64.c

# Architecture-optimized main sources.  Mesa removed the legacy hand-written
# transform assembly; only the SSE4.1 min/max helper remains.
PATHS.asm_s=	mesa/main
.if ${MACHINE} == "amd64" || ${MACHINE} == "i386"
SRCS.asm_s= \
	sse_minmax.c
COPTS.sse_minmax.c+= -msse4.1
.endif

# State tracker sources
PATHS.state_tracker=	mesa/state_tracker
INCLUDES.state_tracker=	glsl mesa/main
SRCS.state_tracker= \
	st_atifs_to_nir.c \
	st_atom_atomicbuf.c \
	st_atom_blend.c \
	st_atom_clip.c \
	st_atom_constbuf.c \
	st_atom_depth.c \
	st_atom_framebuffer.c \
	st_atom_image.c \
	st_atom_msaa.c \
	st_atom_pixeltransfer.c \
	st_atom_rasterizer.c \
	st_atom_sampler.c \
	st_atom_scissor.c \
	st_atom_shader.c \
	st_atom_stipple.c \
	st_atom_storagebuf.c \
	st_atom_tess.c \
	st_atom_texture.c \
	st_atom_viewport.c \
	st_cb_bitmap.c \
	st_cb_clear.c \
	st_cb_copyimage.c \
	st_cb_drawpixels.c \
	st_cb_drawtex.c \
	st_cb_eglimage.c \
	st_cb_feedback.c \
	st_cb_flush.c \
	st_cb_rasterpos.c \
	st_cb_readpixels.c \
	st_cb_texture.c \
	st_context.c \
	st_copytex.c \
	st_debug.c \
	st_draw.c \
	st_draw_feedback.c \
	st_draw_hw_select.c \
	st_extensions.c \
	st_format.c \
	st_gen_mipmap.c \
	st_interop.c \
	st_manager.c \
	st_nir_builtins.c \
	st_nir_lower_alpha_test.c \
	st_nir_lower_builtin.c \
	st_nir_lower_drawpixels.c \
	st_nir_lower_fog.c \
	st_nir_lower_point_size_mov.c \
	st_nir_lower_position_invariant.c \
	st_nir_lower_tex_src_plane.c \
	st_pbo.c \
	st_pbo_compute.c \
	st_program.c \
	st_sampler_view.c \
	st_scissor.c \
	st_shader_cache.c \
	st_texcompress_compute.c \
	st_texture.c \
	st_atom_array.cpp \
	st_glsl_to_nir.cpp

# Program sources
PATHS.program=	mesa/program ../../src/mesa/program
INCLUDES.program=	glsl
SRCS.program= \
	arbprogparse.c \
	prog_cache.c \
	prog_instruction.c \
	prog_parameter.c \
	prog_parameter_layout.c \
	prog_print.c \
	prog_statevars.c \
	prog_to_nir.c \
	program.c \
	program_parse_extra.c \
	symbol_table.c \
	lex.yy.c \
	program_parse.tab.c

# Generated
#.PATH:	${X11SRCDIR.Mesa}/../src/mesa/program
#SRCS.program+= \
#	lex.yy.c


# Run throught all the modules and setup the SRCS and CPPFLAGS etc.
.for _mod_ in ${MESA_SRC_MODULES}

SRCS+=	${SRCS.${_mod_}}

. for _path_ in ${PATHS.${_mod_}}
.PATH:	${X11SRCDIR.Mesa}/src/${_path_}
. endfor

. for _path_ in ${INCLUDES.${_mod_}}
.  for _s in ${SRCS.${_mod_}}
CPPFLAGS.${_s}+=	-I${X11SRCDIR.Mesa}/src/${_path_}
.  endfor
. endfor

.endfor

CPPFLAGS+=	-I${X11SRCDIR.Mesa}/include
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/mesa
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/../src
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/mesa/glapi
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/gallium/include
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/../src/mesa/glapi/glapi
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/../src/mesa/glapi/glapi/gen
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/../src/mesa
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/../src/mesa/main
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/mesa/drivers/dri/common

CPPFLAGS+=	\
	-DPACKAGE_NAME=\"Mesa\" \
	-DPACKAGE_TARNAME=\"mesa\" \
	-DPACKAGE_VERSION=\"${MESA_VER}\" \
	-DPACKAGE_STRING=\"Mesa\ ${MESA_VER}\" \
	-DVERSION=\"${MESA_VER}\" \
	-DPACKAGE_BUGREPORT=\"https://bugs.freedesktop.org/enter_bug.cgi\?product=Mesa\" \
	-DPACKAGE_URL=\"\" \
	-DPACKAGE=\"mesa\" \
	-DHAVE_OPENGL=1 -DHAVE_OPENGL_ES_1=1 -DHAVE_OPENGL_ES_2=1 \

CPPFLAGS+=	\
	-DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 \
	-DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 \
	-DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 \
	-DHAVE___BUILTIN_BSWAP32=1 -DHAVE___BUILTIN_BSWAP64=1 \
	-DHAVE___BUILTIN_CLZ=1 -DHAVE___BUILTIN_CLZLL=1 \
	-DHAVE___BUILTIN_CTZ=1 -DHAVE___BUILTIN_EXPECT=1 \
	-DHAVE___BUILTIN_FFS=1 -DHAVE___BUILTIN_FFSLL=1 \
	-DHAVE___BUILTIN_POPCOUNT=1 -DHAVE___BUILTIN_POPCOUNTLL=1 \
	-DHAVE___BUILTIN_UNREACHABLE=1 -DHAVE_FUNC_ATTRIBUTE_CONST=1 \
	-DHAVE_FUNC_ATTRIBUTE_FLATTEN=1 -DHAVE_FUNC_ATTRIBUTE_FORMAT=1 \
	-DHAVE_FUNC_ATTRIBUTE_MALLOC=1 -DHAVE_FUNC_ATTRIBUTE_PACKED=1 \
	-DHAVE_FUNC_ATTRIBUTE_PURE=1 -DHAVE_FUNC_ATTRIBUTE_RETURNS_NONNULL=1 \
	-DHAVE_FUNC_ATTRIBUTE_UNUSED=1 -DHAVE_FUNC_ATTRIBUTE_VISIBILITY=1 \
	-DHAVE_FUNC_ATTRIBUTE_WARN_UNUSED_RESULT=1 \
	-DHAVE_FUNC_ATTRIBUTE_WEAK=1 -DHAVE_FUNC_ATTRIBUTE_ALIAS=1 \
	-DHAVE_FUNC_ATTRIBUTE_NORETURN=1 -DHAVE_ENDIAN_H=1 -DHAVE_DLADDR=1 \
	-DHAVE_CLOCK_GETTIME=1 -DHAVE_PTHREAD_PRIO_INHERIT=1 \
	-DHAVE_PTHREAD=1 -DHAVE_SYSCONF=1 -DHAVE_STRUCT_TIMESPEC=1 \
	-D__STDC_CONSTANT_MACROS \
	-D__STDC_FORMAT_MACROS \
	-D__STDC_LIMIT_MACROS \
	-DUSE_GCC_ATOMIC_BUILTINS \
	-DNDEBUG \
	-DHAVE_COMPRESSION \
	-DHAVE_SYS_SYSCTL_H \
	-DHAVE_DLFCN_H \
	-DHAVE_STRTOF \
	-DHAVE_REALLOCARRAY \
	-DHAVE_MKOSTEMP \
	-DHAVE_TIMESPEC_GET \
	-DHAVE_STRTOD_L \
	-DHAVE_DL_ITERATE_PHDR \
	-DHAVE_POSIX_MEMALIGN \
	-DHAVE_ZLIB \
	-DHAVE_LIBDRM -DGLX_USE_DRM \
	-DGLX_INDIRECT_RENDERING \
	-DGLX_DIRECT_RENDERING \
	-DGLX_USE_TLS \
	-DHAVE_X11_PLATFORM \
	-DHAVE_DRM_PLATFORM \
	-DENABLE_SHADER_CACHE \
	-DHAVE_MINCORE \
	-DHAVE_NOATEXIT

.if ${MKLLVMRT} != "no"
LLVM_VERSION!=		cd ${NETBSDSRCDIR}/external/apache2/llvm && ${MAKE} -V LLVM_VERSION
HAVE_LLVM_VERSION!=	expr ${LLVM_VERSION:R:R} \* 256 + ${LLVM_VERSION:R:E} \* 16
CPPFLAGS+=	\
	-DHAVE_LLVM=${HAVE_LLVM_VERSION}
CPPFLAGS+=	-DLLVM_AVAILABLE -DDRAW_LLVM_AVAILABLE
CXXFLAGS+=	-fno-rtti
.endif

CPPFLAGS+=	\
	-DLITTLEENDIAN_CPU

.include "../asm.mk"

CPPFLAGS+=	\
	-DHAVE_LIBDRM -DGLX_USE_DRM -DGLX_INDIRECT_RENDERING -DGLX_DIRECT_RENDERING -DHAVE_ALIAS -DMESA_EGL_NO_X11_HEADERS

CPPFLAGS+=	\
	-DYYTEXT_POINTER=1

CFLAGS+=	-fvisibility=hidden -fno-strict-aliasing -fno-builtin-memcmp -fcommon

.include "libGL/mesa-ver.mk"
