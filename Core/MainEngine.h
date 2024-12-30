#pragma once

#include "EngineBase.h"

#include "Model.h"
#include "GeometryGenerator.h"

class MainEngine : public EngineBase
{
public:
	MainEngine();

	virtual void Initialize() override;
	virtual void Update(float dt) override;
	virtual void Render() override;
	virtual void UpdateGUI() override;

private:

	std::vector<Model> models;

};