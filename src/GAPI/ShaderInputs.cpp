#include "ShaderInputs.h"
#include "Texture2d.h"

namespace engine
{
	namespace graphic
	{
		ShaderInputConstant::~ShaderInputConstant()
		{
			if (m_constant)
			{
				delete m_constant;
			}
			m_constant = nullptr;
		}
	}
}