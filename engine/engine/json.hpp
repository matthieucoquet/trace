#pragma once
#include <filesystem>

namespace sdf_editor
{
struct Scene;

class Json
{
public:
	Json(Scene& scene, std::filesystem::path path);
	void parse(Scene& scene);
	void write_to_file(const Scene& scene);
private:
	std::filesystem::path m_path;
};

}