#pragma once
#include "core/system.hpp"
#include <filesystem>

namespace sdf_editor
{
struct Scene;

class Json_system : public System
{
public:
	Json_system(Scene& scene, std::filesystem::path path);
	Json_system(const Json_system& other) = delete;
	Json_system(Json_system&& other) = delete;
	Json_system& operator=(const Json_system& other) = delete;
	Json_system& operator=(Json_system&& other) = delete;
	~Json_system() override = default;

	void parse(Scene& scene);
	void write_to_file(const Scene& scene);

	void step(Scene& scene) override final;
private:
	std::filesystem::path m_path;
};

}