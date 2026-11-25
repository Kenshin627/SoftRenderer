#include "Material.h"

void Material::ConfigureOpacity(float dissolve, const std::shared_ptr<Texture>& map, char channel, AlphaMode mode)
{
	opacity = glm::clamp(dissolve, 0.0f, 1.0f);
	opacityMap = map;
	opacityChannel = channel;
	if (opacityMap && channel == 'm' && !opacityMap->HasAlpha()) opacityChannel = 'l';
	if (mode != AlphaMode::Auto) alphaMode = mode;
	else if (opacity == 0.0f) alphaMode = AlphaMode::Mask;
	else if (opacityMap) alphaMode = opacityMap->ClassifyOpacity(opacityChannel, opacity);
	else alphaMode = opacity < 1.0f ? AlphaMode::Blend : AlphaMode::Opaque;
}

AlphaMode Material::Mode(AlphaMode overrideMode) const
{
	if (overrideMode == AlphaMode::Auto || alphaMode == AlphaMode::Opaque)
		return alphaMode;
	return overrideMode;
}

float Material::SampleOpacity(const glm::vec2& uv, TextureFilter filter, const glm::vec2& dx, const glm::vec2& dy, int maxAnisotropy) const
{
	if (!opacityMap) return opacity;
	return opacity * Texture::OpacityChannel(opacityMap->SampleRgba(uv, filter, dx, dy, false, maxAnisotropy), opacityChannel);
}
