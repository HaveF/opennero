//---------------------------------------------------
// Name: OpenNero : Agent
// Desc: Interface for an AI agent acting in the world
//---------------------------------------------------

#ifndef _OPENNERO_AI_AGENT_H_
#define _OPENNERO_AI_AGENT_H_

#include "core/ONTime.h"
#include "ai/AI.h"
#include "ai/AIObject.h"
#include "game/objects/TemplatedObject.h"
#include "scripting/scriptIncludes.h"
#include "scripting/scripting.h"

namespace OpenNero
{

	/// @cond
    BOOST_PTR_DECL(AIObject);
    /// @endcond

    using namespace boost;

    /// AI agent acting in the world
    class AgentBrain : public TemplatedObject
    {
            AIObjectWPtr mBody; ///< AIObject where this brain is attached
        public:
            /// constructor
            AgentBrain() : mBody() {}

            /// destructor
            virtual ~AgentBrain() {}

            /// called right before the agent is born
            virtual bool initialize(const AgentInitInfo& init) = 0;

            /// called for agent to take its first step
            virtual Actions start(const TimeType& time, const Sensors& sensors) = 0;

            /// act based on time, sensor arrays, and last reward
            virtual Actions act(const TimeType& time, const Sensors& sensors, const Reward& reward) = 0;

            /// called to tell agent about its last reward
            virtual bool end(const TimeType& time, const Reward& reward) = 0;

            /// called right before the agent dies
            virtual bool destroy() = 0;

            /// set the body associated with this agent
            virtual void SetBody(AIObjectPtr body) { mBody = body; }

            /// get the body associated with this agent
            virtual AIObjectPtr GetBody() { return mBody.lock(); }

            /// get the shared simulation data for adjusting pose
            SimEntityData* GetSharedData() { return GetBody()->GetSharedData(); }

            std::string name; ///< name of this brain
    };

    /// shared pointer to an AgentBrain
    typedef shared_ptr<AgentBrain> AgentBrainPtr;

    /// C++ interface for Python-side AgentBrain
    class PyAgentBrain : public AgentBrain, public TryWrapper<AgentBrain>
    {
        public:
            /// called right before the agent is born
            bool initialize(const AgentInitInfo& init);

            /// called for agent to take its first step
            Actions start(const TimeType& time, const Sensors& sensors);

            /// act based on time, sensor arrays, and last reward
            Actions act(const TimeType& time, const Sensors& sensors,
                        const Reward& reward);

            /// called to tell agent about its last reward
            bool end(const TimeType& time, const Reward& reward);

            /// called right before the agent dies
            bool destroy();

            /// use a template to initialize this object
			bool LoadFromTemplate(ObjectTemplatePtr t, const SimEntityData& data);
    };

    /// shared pointer to a PyAgentBrain
    typedef shared_ptr<PyAgentBrain> PyAgentBrainPtr;
}

#endif