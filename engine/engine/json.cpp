#include "json.hpp"
#include "core/scene.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <glm/glm.hpp>

using json = nlohmann::json;

namespace sdf_editor
{

static json to_json(const glm::vec3& vec) {
	return json({ vec.x, vec.y, vec.z });
}
static glm::vec3 to_vec3(const json& j) {
	return glm::vec3{ j[0], j[1], j[2] };
}

static json to_json(const glm::quat& q) {
	return json::array({ q.w, q.x, q.y, q.z });
}
static glm::quat to_quat(const json& j) {
	return glm::quat{ j[0], j[1], j[2], j[3] };
}

Json::Json(std::filesystem::path path):
	m_path(std::move(path))
{}

void Json::parse(Scene& scene)
{
	std::ifstream stream(m_path);
	json j;
	stream >> j;

	const json& objects = j["objects"];
	scene.objects.reserve(objects.size());
	for (const auto& object : objects)
	{
		scene.objects.emplace_back(Object{
			.name = object["name"].get<std::string>(),
			.position = to_vec3(object["position"]),
			.rotation = to_quat(object["rotation"]),
			.scale = object["scale"].get<float>(),
			.group_id = object["group_id"].get<size_t>()
		});
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

	for (auto& object : scene.objects) {
		glm::mat4 model_to_world = glm::translate(object.position) * glm::toMat4(object.rotation) * glm::scale(glm::vec3(object.scale));
		scene.objects_transform.emplace_back(glm::inverse(model_to_world));
	}
}

void Json::write_to_file(const Scene& scene)
{
	json objects;
	for (const auto& object : scene.objects)
	{
		if (object.group_id == 0) {
			continue; // It's hands
		}
		objects.push_back(json{ 
			{ "name", object.name },
			{ "position", to_json(object.position) },
			{ "rotation", to_json(object.rotation) },
			{ "scale", object.scale },
			{ "group_id", object.group_id },
		});
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
	j["objects"] = objects;
	j["materials"] = materials;
	j["lights"] = lights;

	std::ofstream ouput(m_path);
	ouput << std::setw(4) << j << std::endl;
}

}