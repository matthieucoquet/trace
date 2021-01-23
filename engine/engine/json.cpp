#include "json.hpp"
#include "core/scene.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <glm/glm.hpp>

using json = nlohmann::json;

namespace sdf_editor
{

// vec3 <-> json
static json to_json(const glm::vec3& vec) {
	return json({ vec.x, vec.y, vec.z });
}
static glm::vec3 to_vec3(const json& j) {
	return glm::vec3{ j[0], j[1], j[2] };
}

// quat <-> json
static json to_json(const glm::quat& q) {
	return json::array({ q.w, q.x, q.y, q.z });
}
static glm::quat to_quat(const json& j) {
	return glm::quat{ j[0], j[1], j[2], j[3] };
}

// entity <-> json
static json to_json(const Entity& e) {
	json j{
			{ "name", e.name },
			{ "position", to_json(e.local_transform.position) },
			{ "rotation", to_json(e.local_transform.rotation) },
			{ "scale", e.scale },
			{ "group_id", e.group_id },
	};
	json children;
	for (const auto& child : e.children)
	{
		children.push_back(to_json(child));
	}
	j["children"] = children;
	return j;
}

static Entity to_entity(const json& j) {
	Entity entity{
			.name = j["name"].get<std::string>(),
			.local_transform = Transform{
				.position = to_vec3(j["position"]),
				.rotation = to_quat(j["rotation"])
			},
			.scale = j["scale"].get<float>(),
			.group_id = j["group_id"].get<size_t>()
	};

	const json& children = j["children"];
	if (children.is_array()) {
		entity.children.reserve(children.size());
		for (const auto& child : children)
		{
			entity.children.push_back(to_entity(child));
		}
	}
	return entity;
}

Json::Json(Scene& scene, std::filesystem::path path):
	m_path(std::move(path))
{
	parse(scene);
}

void Json::parse(Scene& scene)
{
	std::ifstream stream(m_path);
	json j;
	stream >> j;

	const json& entities = j["entities"];
	scene.entities.reserve(entities.size());
	for (const auto& entity : entities)
	{
		scene.entities.emplace_back(to_entity(entity));
	}

	const json& materials = j["materials"];
	scene.materials.reserve(materials.size());
	for (const auto& material : materials)
	{
		scene.materials.push_back(Material{ .color = to_vec3(material["color"]) });
	}

	const json& lights = j["lights"];
	scene.lights.reserve(lights.size());
	for (const auto& light : lights)
	{
		scene.lights.push_back(Light{ .position = to_vec3(light["position"]), .color = to_vec3(light["color"]) });
	}
}

void Json::write_to_file(const Scene& scene)
{
	json entities;
	for (const auto& entity : scene.entities)
	{
		if (entity.group_id == 0) {
			continue; // It's hands
		}
		entities.push_back(to_json(entity));
	}

	json materials;
	for (const auto& material : scene.materials)
	{
		materials.push_back(json{ { "color", to_json(material.color) } });
	}

	json lights;
	for (const auto& light : scene.lights)
	{
		lights.push_back(json{ { "position", to_json(light.position) }, { "color", to_json(light.color) } });
	}

	json j;
	j["entities"] = entities;
	j["materials"] = materials;
	j["lights"] = lights;

	std::ofstream ouput(m_path);
	ouput << std::setw(4) << j << std::endl;
}

}