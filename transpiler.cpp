#include <cstring>
#include <filesystem>
#include <iostream>

#include <argparse/argparse.hpp>
#include <stdexcept>

#include "slcross.hpp"
#include "thirdparty/glob.hpp"
#include "read_entire_file.hpp"



struct Arguments : public argparse::Args {
	std::string& infile = arg("input-file", "The file to load a shader from.");
	std::optional<std::string>& outfile = arg("output-file", "The file to store the transpiled shader to.").set_default(std::optional<std::string>{});
	std::optional<slcross::language>& inlang = kwarg("i,input-language", "The language of the input file (often automatically deduced from its extension).");
	std::string& entry_point = kwarg("e,ep,entry,entry-point", "The name of the entry point in the input file").set_default("main");
	std::optional<slcross::language>& outlang = kwarg("l,output-language", "The language of the output file (often automatically deduced from its extension).");
	std::optional<slcross::shader_stage>& stage = kwarg("s,stage", "The shader stage to transpile (only necessary for glsl)");

	bool& optimize = flag("O,optimize", "Weather or not the shader should be optimized before outputting it").set_default(false);
	bool& canonicalize = flag("C,canonicalize", "Weather or not the shader should be canonicalized (by round tripping it through glsl). Enabling this option may fix some output generators.").set_default(false);

	// Target specific settings
	bool& glsl_target_vulkan = kwarg("glsl-target-vulkan", "Should GLSL use the vulkan shader model?").set_default(true);
	bool& glsl_target_web = kwarg("glsl-target-web", "Should we be targeting OpenGL ES?").set_default(false);
	size_t& glsl_version = kwarg("glsl-version", "The GLSL version to target.").set_default(450);
	size_t& hlsl_shader_model = kwarg("hlsl-shader-model", "The HLSL shader model to target.").set_default(50);
	bool& msl_target_ios = kwarg("msl-target-ios", "Should we be targeting iOS?").set_default(false);
	std::vector<std::string>& slang_includes = kwarg("slang-includes", "Slang files to also import into the build").set_default(std::vector<std::string>{});
};

std::vector<std::string_view> split(std::string_view str, const std::string_view delimiter) {
	std::vector<std::string_view> result;
	size_t start = 0;
	size_t end = 0;

	while (true) {
		end = str.find(delimiter, start);
		if (end == std::string_view::npos) {
			if (start < str.length())
				result.push_back(str.substr(start));
			break;
		}

		if (start < end)
			result.push_back(str.substr(start, end - start));


		start = end + delimiter.length();
	}

	return result;
}


struct metadata {
	std::string file;
	slcross::language lang;
	std::optional<slcross::shader_stage> stage = {};
};
metadata parse_file_metadata(std::string_view file, const std::optional<slcross::language>& lang, const std::optional<slcross::shader_stage>& stage = {}) {
	auto generic_error = "\nIf additional information is encoded in a file name it must be of the form `file:language:stage`";
	metadata res;
	auto subs = split(file, ":");
	res.file = subs[0];

	if(lang.has_value()) res.lang = *lang;
	else if(subs.size() >= 2) {
		auto lang = magic_enum::enum_cast<slcross::language>(subs[1], magic_enum::case_insensitive);
		if(!lang.has_value()) throw std::runtime_error("Invalid language: " + std::string{subs[1]} + generic_error);
		res.lang = *lang;
	} else {
		auto langStr = std::filesystem::path(res.file).extension().string();
		if(langStr.size()) langStr = langStr.substr(1);

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
			if(args.infile == ".") throw std::runtime_error("When reading from standard input an output file must be specified!");
			else if(!args.outlang.has_value()) throw std::runtime_error("If no output file is specified an output language must be specified using --output-language!");
			else outfile = std::filesystem::path(args.infile).replace_extension(magic_enum::enum_name(*args.outlang));
		} else outfile = *args.outfile;

		auto in = parse_file_metadata(args.infile, args.inlang);
		if(in.stage.has_value())
			std::cerr << "Warning: Specifying a stage on an input file has no effect!" << std::endl;
		auto out = parse_file_metadata(outfile, args.outlang, args.stage);

		if(out.lang == slcross::language::glsl && !out.stage.has_value())
			throw std::runtime_error("When targeting glsl a shader stage must be specified using --stage!");

		// Remove any whitespace from the entry point
		auto end = std::remove_if(args.entry_point.begin(), args.entry_point.end(), [](char c) {
			return std::isspace(c);
		});
		if(end != args.entry_point.end()) args.entry_point.erase(end);




		slcross::spirv module;

		switch(in.lang) {
		break; case slcross::language::glsl:
#ifdef SLCROSS_ENABLE_READING_GLSL
			module = slcross::glsl::parse_from_memory(out.stage.value(), slcross::read_entire_file_as_string(in.file), args.entry_point);
#else
			throw std::runtime_error("Parsing GLSL has been disabled in this build!");
#endif
		break; case slcross::language::wgsl:
#ifdef SLCROSS_ENABLE_WGSL
			module = slcross::wgsl::parse_from_memory(slcross::read_entire_file_as_string(in.file), args.glsl_target_vulkan, in.file);
#else
			throw std::runtime_error("Parsing WGSL has been disabled in this build!");
#endif
#ifdef SLCROSS_ENABLE_SLANG
		break; case slcross::language::hlsl:
			std::cerr << "Warning: HLSL input is not supported... parsing as a slang shader!" << std::endl;
			[[fallthrough]];
		case slcross::language::slang: {
			std::vector<std::string> globbed_slang_includes;
			for(auto path: args.slang_includes)
				for(auto glob: glob::rglob(path))
					globbed_slang_includes.emplace_back(std::filesystem::relative(glob));

			auto session = slcross::slang::create_session();
			for(auto path: globbed_slang_includes) {
				auto module = std::filesystem::path(path).filename().replace_extension().string();
				if(!inject_module_from_memory(session, slcross::read_entire_file_as_string(path), path, module))
					throw std::runtime_error("Failed to parse slang include: " + path);
			}
			auto mod = std::filesystem::path(in.file).filename().replace_extension().string();
			if(in.file == ".") mod = "generated";
			module = slcross::slang::parse_from_memory(session, slcross::read_entire_file_as_string(in.file), args.entry_point, in.file, mod);
		}
#else
		break; case slcross::language::hlsl: [[fallthrough]];
		case slcross::language::slang:
			throw std::runtime_error("Parsing Slang/HLSL has been disabled in this build!");
#endif
		break; case slcross::language::msl:
			throw std::runtime_error("Parsing Metal shaders is not currently supported!");

		break; case slcross::language::spirv: {
			module = slcross::read_entire_file_as<uint32_t>(in.file);
		}
		}




		slcross::validate(module);
		if(args.optimize) module = slcross::optimize(module);

		if(args.canonicalize)
#ifdef SLCROSS_ENABLE_READING_GLSL
			module = slcross::glsl::canonicalize(module, out.stage.value());
#else
			throw std::runtime_error("Canonicalization (GLSL) has been disabled in this build!");
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
		break; case slcross::language::spirv:

			if(out.file == ".") std::cout.write((char*)module.data(), module.size() * sizeof(uint32_t));
			else std::ofstream(out.file).write((char*)module.data(), module.size() * sizeof(uint32_t));
			return 0;
		}

		if(out.file == ".") std::cout << generated << std::endl;
		else std::ofstream(out.file) << generated << std::flush;
	} catch(const std::runtime_error& e) {
		std::cerr << "Error " << e.what() << "\n"
			<< "\n"
			<< "Try using the following form: " << argv[0] << " <input-file> -l <output-language>\n"
			<< "Additionally information can be encoded along with a file as long as it follows the\n"
			<< "following form: `<file>:<language>:<stage>`. Or use --help for more details!" << std::endl;
	}
}