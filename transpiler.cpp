#include <filesystem>
#include <ios>
#include <iostream>

#include <argparse/argparse.hpp>
#include <stdexcept>

#include "slcross.hpp"



struct Arguments : public argparse::Args {
	std::string& infile = arg("input-file", "The file to load a shader from.");
	std::optional<std::string>& outfile = arg("output-file", "The file to store the transpiled shader to.").set_default(std::optional<std::string>{});
	std::optional<slcross::language>& inlang = kwarg("i,input-language", "The language of the input file (often automatically deduced from its extension).");
	std::string& entry_point = kwarg("e,ep,entry,entry-point", "The name of the entry point in the input file").set_default("main");
	std::optional<slcross::language>& outlang = kwarg("l,output-language", "The language of the output file (often automatically deduced from its extension).");
	std::optional<slcross::shader_stage>& stage = kwarg("s,stage", "The shader stage to transpile (only nessicary for glsl)");

	bool& optimize = flag("O,optimize", "Weather or not the shader should be optimized before outputting it").set_default(false);
	bool& canonicalize = flag("C,canonicalize", "Weather or not the shader should be canonicalized (by round tripping it through glsl). Enabling this option may fix some output generators.").set_default(false);

	// Target specific settings
	bool& glsl_target_vulkan = kwarg("glsl-target-vulkan", "Should GLSL use the vulkan shader model?").set_default(true);
	bool& glsl_target_web = kwarg("glsl-target-web", "Should we be targeting OpenGL ES?").set_default(false);
	size_t& glsl_version = kwarg("glsl-version", "The GLSL version to target.").set_default(450);
	size_t& hlsl_shader_model = kwarg("hlsl-shader-model", "The HLSL shader model to target.").set_default(50);
	bool& msl_target_ios = kwarg("msl-target-ios", "Should we be targeting iOS?").set_default(false);
};



std::string read_entire_file(const std::string& path) {
	std::ifstream fin(path);
	if(!fin) throw std::runtime_error("Failed to open file: " + path);

	fin.seekg(0, std::ios::end);
	std::string out(fin.tellg(), '\0');
	fin.seekg(0, std::ios::beg);
	fin.read(out.data(), out.size());
	return out;
}

std::vector<std::string_view> split(const std::string_view s, const std::string_view delimiter) {
	std::vector<std::string_view> tokens;
	size_t pos, old;
	for (pos = 0, old = 0; (pos = s.find(delimiter, pos)) != std::string::npos; old = pos)
		tokens.emplace_back(s.substr(old, pos));

	if(tokens.empty()) pos = 0;
	tokens.push_back(s.substr(pos));

	return tokens;
}

struct parse_file_res {
	std::string file;
	slcross::language lang;
	std::optional<slcross::shader_stage> stage = {};
};
parse_file_res parse_file(std::string_view file, const std::optional<slcross::language>& lang, const std::optional<slcross::shader_stage>& stage = {}) {
	auto generic_error = "\nIf additional information is encoded in a file name it must be of the form `file:language:stage`";
	parse_file_res res;
	auto subs = split(file, ":");
	res.file = subs[0];

	if(lang.has_value()) res.lang = *lang;
	else if(subs.size() >= 2) {
		auto lang = magic_enum::enum_cast<slcross::language>(subs[1], magic_enum::case_insensitive);
		if(!lang.has_value()) throw std::runtime_error("Invalid language: " + std::string{subs[1]} + generic_error);
		res.lang = *lang;
	} else {
		auto langStr = std::filesystem::path(res.file).extension().string().substr(1);
		auto lang = magic_enum::enum_cast<slcross::language>(langStr, magic_enum::case_insensitive);
		if(!lang.has_value()) throw std::runtime_error("Invalid language `" + langStr + "` please provide a valid language!" + generic_error);
		res.lang = *lang;
	}

	if(stage.has_value()) res.stage = *stage;
	else if(subs.size() >= 3) {
		auto stage = magic_enum::enum_cast<slcross::shader_stage>(subs[2], magic_enum::case_insensitive);
		if(!stage.has_value()) throw std::runtime_error("Invalid shader stage: " + std::string{subs[2]} + generic_error);
		res.stage = *stage;
	}

	if(subs.size() >= 4) throw std::runtime_error(generic_error);
	return res;
}

int main(int argc, char** argv) {
	try {
		auto args = argparse::parse<Arguments>(argc, argv, true);
		std::string outfile;
		if(!args.outfile.has_value()) {
			if(!args.outlang.has_value()) throw std::runtime_error("If no output file is specified an output language must be specified using --output-language!");
			else outfile = std::filesystem::path(args.infile).replace_extension(magic_enum::enum_name(*args.outlang));
		} else outfile = *args.outfile;

		auto in = parse_file(args.infile, args.inlang);
		if(in.stage.has_value())
			std::cerr << "Warning: Specifying a stage on an input file has no effect!";
		auto out = parse_file(outfile, args.outlang, args.stage);

		if(out.lang == slcross::language::glsl && !out.stage.has_value())
			throw std::runtime_error("When targeting glsl a shader stage must be specified using --stage!");



		slcross::spirv module;

		switch(in.lang) {
		break; case slcross::language::glsl:
#ifdef SLCROSS_ENABLE_READING_GLSL
			module = slcross::glsl::parse_from_memory(out.stage.value(), read_entire_file(in.file), args.entry_point);
#else
			throw std::runtime_error("Parsing GLSL has been disabled in this build!");
#endif
		break; case slcross::language::wgsl:
#ifdef SLCROSS_ENABLE_WGSL
			module = slcross::wgsl::parse_from_memory(read_entire_file(in.file), args.glsl_target_vulkan, in.file);
#else
			throw std::runtime_error("Parsing WGSL has been disabled in this build!");
#endif
#ifdef SLCROSS_ENABLE_SLANG
		break; case slcross::language::hlsl:
			std::cerr << "HLSL input is not supported... parsing as a slang shader!" << std::endl;
			[[fallthrough]];
		case slcross::language::slang:
			module = slcross::wgsl::parse_from_memory(read_entire_file(in.file), true, in.file);
#else
		break; case slcross::language::hlsl: [[fallthrough]];
		case slcross::language::slang:
			throw std::runtime_error("Parsing Slang has been disabled in this build!");
#endif
		break; case slcross::language::msl:
			throw std::runtime_error("Parsing Metal shaders is not currently supported!");
		}

		slcross::validate(module);
		if(args.optimize) module = slcross::optimize(module);

		if(args.canonicalize)
#ifdef SLCROSS_ENABLE_READING_GLSL
			module = slcross::glsl::canonicalize(module, out.stage.value());
#else
			throw std::runtime_error("Canonicalization has been disabled in this build!");
#endif



		std::string generated;
		switch(out.lang){
		break; case slcross::language::glsl:
			generated = slcross::glsl::generate(module, out.stage.value(), args.entry_point, args.glsl_target_vulkan, args.glsl_target_web, args.glsl_version);
		break; case slcross::language::hlsl:
			generated = slcross::hlsl::generate(module, args.hlsl_shader_model);
		break; case slcross::language::wgsl:
#ifdef SLCROSS_ENABLE_WGSL
			generated = slcross::wgsl::generate(module);
#else
			throw std::runtime_error("Outputting WGSL has been disabled in this build!");
#endif
		break; case slcross::language::slang:
			throw std::runtime_error("Outputting Slang shaders is not currently supported!");
		break; case slcross::language::msl:
			generated = slcross::msl::generate(module, args.msl_target_ios);
		}

		std::ofstream(out.file) << generated << std::flush;
	} catch(const std::runtime_error& e) {
		std::cerr << "Error " << e.what() << "\n"
			<< "\n"
			<< "Try using the following form: " << argv[0] << " <input-file> -l <output-language>\n"
			<< "Additionally information can be encoded along with a file as long as it follows the\n"
			<< "following form: `<file>:<language>:<stage>`. Or use --help for more details!" << std::endl;
	}
}