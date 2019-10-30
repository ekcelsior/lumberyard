﻿/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <MCore/Source/AzCoreConversions.h>

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/BlendTreeFootIKNode.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/TransformData.h>

#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/EventDataFootIK.h>
#include <EMotionFX/Source/MotionEvent.h>

#include <Integration/AnimationBus.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeFootIKNode, AnimGraphAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeFootIKNode::UniqueData, AnimGraphObjectUniqueDataAllocator, 0)

    BlendTreeFootIKNode::BlendTreeFootIKNode()
        : AnimGraphNode()
    {
        // Setup the input ports.
        InitInputPorts(11);
        SetupInputPort("Pose", INPUTPORT_POSE, AttributePose::TYPE_ID, PORTID_INPUT_POSE);
        SetupInputPortAsNumber("Foot Height", INPUTPORT_FOOTHEIGHT, PORTID_INPUT_FOOTHEIGHT);
        SetupInputPortAsNumber("Ray Length", INPUTPORT_RAYCASTLENGTH, PORTID_INPUT_RAYCASTLENGTH);
        SetupInputPortAsNumber("Max Hip Adj.", INPUTPORT_MAXHIPADJUST, PORTID_INPUT_MAXHIPADJUST);
        SetupInputPortAsNumber("Max Foot Adj.", INPUTPORT_MAXFOOTADJUST, PORTID_INPUT_MAXFOOTADJUST);
        SetupInputPortAsNumber("Leg Blend Speed", INPUTPORT_IKBLENDSPEED, PORTID_INPUT_IKBLENDSPEED);
        SetupInputPortAsNumber("Foot Blend Speed", INPUTPORT_FOOTBLENDSPEED, PORTID_INPUT_FOOTBLENDSPEED);
        SetupInputPortAsNumber("Hip Blend Speed", INPUTPORT_HIPBLENDSPEED, PORTID_INPUT_HIPBLENDSPEED);
        SetupInputPortAsBool("Adjust Hip", INPUTPORT_ADJUSTHIP, PORTID_INPUT_ADJUSTHIP);
        SetupInputPortAsBool("Lock Feet", INPUTPORT_FOOTLOCK, PORTID_INPUT_FOOTLOCK);
        SetupInputPortAsNumber("Weight", INPUTPORT_WEIGHT, PORTID_INPUT_WEIGHT);

        // Setup the output ports.
        InitOutputPorts(1);
        SetupOutputPortAsPose("Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }

    void BlendTreeFootIKNode::Reinit()
    {
        if (!mAnimGraph)
        {
            return;
        }

        AnimGraphNode::Reinit();

        const size_t numInstances = mAnimGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = mAnimGraph->GetAnimGraphInstance(i);
            UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueNodeData(this));
            if (!uniqueData)
            {
                continue;
            }

            uniqueData->m_mustUpdate = true;
            animGraphInstance->UpdateUniqueData();
        }
    }

    bool BlendTreeFootIKNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();
        Reinit();
        return true;
    }

    const char* BlendTreeFootIKNode::GetPaletteName() const
    {
        return "Footplant IK";
    }

    AnimGraphObject::ECategory BlendTreeFootIKNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_CONTROLLERS;
    }

    float BlendTreeFootIKNode::GetFootHeightOffset(AnimGraphInstance* animGraphInstance) const
    {
        const float actorInstanceScale = animGraphInstance->GetActorInstance()->GetWorldSpaceTransform().mScale.GetZ();
        MCore::AttributeFloat* input = GetInputFloat(animGraphInstance, INPUTPORT_FOOTHEIGHT);
        return input ? input->GetValue() * actorInstanceScale : m_footHeightOffset * actorInstanceScale;
    }

    float BlendTreeFootIKNode::GetRaycastLength(AnimGraphInstance* animGraphInstance) const
    {
        MCore::AttributeFloat* input = GetInputFloat(animGraphInstance, INPUTPORT_RAYCASTLENGTH);
        return input ? input->GetValue() : m_raycastLength;
    }

    float BlendTreeFootIKNode::GetMaxHipAdjustment(AnimGraphInstance* animGraphInstance) const
    {
        const float actorInstanceScale = animGraphInstance->GetActorInstance()->GetWorldSpaceTransform().mScale.GetZ();
        MCore::AttributeFloat* input = GetInputFloat(animGraphInstance, INPUTPORT_MAXHIPADJUST);
        return input ? input->GetValue() * actorInstanceScale : m_maxHipAdjustment * actorInstanceScale;
    }

    float BlendTreeFootIKNode::GetMaxFootAdjustment(AnimGraphInstance* animGraphInstance) const
    {
        const float actorInstanceScale = animGraphInstance->GetActorInstance()->GetWorldSpaceTransform().mScale.GetZ();
        MCore::AttributeFloat* input = GetInputFloat(animGraphInstance, INPUTPORT_MAXFOOTADJUST);
        return input ? input->GetValue() * actorInstanceScale : m_maxFootAdjustment * actorInstanceScale;
    }

    float BlendTreeFootIKNode::GetIKBlendSpeed(AnimGraphInstance* animGraphInstance) const
    {
        MCore::AttributeFloat* input = GetInputFloat(animGraphInstance, INPUTPORT_IKBLENDSPEED);
        return input ? input->GetValue() : m_ikBlendSpeed;
    }

    float BlendTreeFootIKNode::GetFootBlendSpeed(AnimGraphInstance* animGraphInstance) const
    {
        MCore::AttributeFloat* input = GetInputFloat(animGraphInstance, INPUTPORT_FOOTBLENDSPEED);
        return input ? input->GetValue() : m_footBlendSpeed;
    }

    float BlendTreeFootIKNode::GetHipBlendSpeed(AnimGraphInstance* animGraphInstance) const
    {
        MCore::AttributeFloat* input = GetInputFloat(animGraphInstance, INPUTPORT_HIPBLENDSPEED);
        return input ? input->GetValue() : m_hipBlendSpeed;
    }

    bool BlendTreeFootIKNode::GetAdjustHip(AnimGraphInstance* animGraphInstance) const
    {
        return HasConnectionAtInputPort(INPUTPORT_ADJUSTHIP) ? GetInputNumberAsBool(animGraphInstance, INPUTPORT_ADJUSTHIP) : m_adjustHip;
    }

    bool BlendTreeFootIKNode::GetFootLock(AnimGraphInstance* animGraphInstance) const
    {
        return HasConnectionAtInputPort(INPUTPORT_FOOTLOCK) ? GetInputNumberAsBool(animGraphInstance, INPUTPORT_FOOTLOCK) : m_footLock;
    }

    // Inititalize the leg by looking up joint indices from their names etc.
    bool BlendTreeFootIKNode::InitLeg(LegId legId, const AZStd::string& footJointName, const AZStd::string& toeJointName, AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        const ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        const Actor* actor = actorInstance->GetActor();
        const Skeleton* skeleton = actor->GetSkeleton();

        Leg& leg = uniqueData->m_legs[legId];

        // Try to find the joint index.
        const Node* footJoint = skeleton->FindNodeAndIndexByName(footJointName, leg.m_jointIndices[LegJointId::Foot]);
        if (!footJoint)
        {
            AZ_Error("EMotionFX", false, "Anim graph footplant IK node '%s' cannot find foot joint named '%s'.", GetName(), footJointName.c_str());
            return false;
        }
        leg.m_footHeight = actorInstance->GetTransformData()->GetBindPose()->GetModelSpaceTransform(leg.m_jointIndices[LegJointId::Foot]).mPosition.GetZ();

        // Now grab the parent, assuming this is the knee.
        Node* knee = footJoint->GetParentNode();
        if (!knee)
        {
            AZ_Error("EMotionFX", false, "Anim graph footplant IK node '%s' cannot find knee/lower leg joint as the foot has no parent.", GetName());
            return false;
        }
        leg.m_jointIndices[LegJointId::Knee] = knee->GetNodeIndex();

        // Get the upper leg, assuming this is the parent of the knee.
        Node* upperLeg = knee->GetParentNode();
        if (!upperLeg)
        {
            AZ_Error("EMotionFX", false, "Anim graph footplant IK node '%s' cannot find upper leg joint as the knee/lower leg has no parent.", GetName());
            return false;
        }
        leg.m_jointIndices[LegJointId::UpperLeg] = upperLeg->GetNodeIndex();

        // Find the toe joint.
        const Node* toeJoint = skeleton->FindNodeAndIndexByName(toeJointName, leg.m_jointIndices[LegJointId::Toe]);
        if (!toeJoint)
        {
            AZ_Error("EMotionFX", false, "Anim graph footplant IK node '%s' cannot find toe joint named '%s'", GetName(), toeJointName.c_str());
            return false;
        }
        leg.m_jointIndices[LegJointId::Toe] = toeJoint->GetNodeIndex();

        leg.m_toeHeight = actorInstance->GetTransformData()->GetBindPose()->GetModelSpaceTransform(leg.m_jointIndices[LegJointId::Toe]).mPosition.GetZ();
        leg.m_weight = 0.0f;
        leg.m_targetWeight = 0.0f;
        leg.DisableFlag(LegFlags::FootDown);
        leg.DisableFlag(LegFlags::ToeDown);

        return true;
    }

    // Lookup some joint indices and check if our configuration is valid.
    void BlendTreeFootIKNode::UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        if (uniqueData->m_mustUpdate)
        {
            const ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            const Actor* actor = actorInstance->GetActor();
            const Skeleton* skeleton = actor->GetSkeleton();

            uniqueData->m_mustUpdate = false;
            uniqueData->m_isValid = false;
            uniqueData->m_mustUpdate = true;

            // Initialize the legs.
            if (!InitLeg(LegId::Left, m_leftFootJointName, m_leftToeJointName, animGraphInstance, uniqueData) || !InitLeg(LegId::Right, m_rightFootJointName, m_rightToeJointName, animGraphInstance, uniqueData))
            {
                return;
            }

            // Try to find the hip joint.
            if (m_hipJointName.empty() || !skeleton->FindNodeAndIndexByName(m_hipJointName, uniqueData->m_hipJointIndex))
            {
                AZ_Error("EMotionFX", false, "Anim graph footplant IK node '%s' cannot find hip joint named '%s'", GetName(), m_hipJointName.c_str());
                return;
            }

            uniqueData->m_isValid = true;
            uniqueData->m_mustUpdate = false;
        }
    }

    // Solve the two joint IK by calculating the new knee position (outMidPos).
    bool BlendTreeFootIKNode::Solve2LinkIK(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& goal, const AZ::Vector3& bendDir, AZ::Vector3* outMidPos)
    {
        const AZ::Vector3 localGoal = goal - posA;
        const float distToTarget = MCore::SafeLength(localGoal);

        const AZ::Vector3 joint1 = posB - posA;
        const AZ::Vector3 joint2 = posC - posB;
        float lengthA = MCore::SafeLength(joint1);
        float lengthB = MCore::SafeLength(joint2);

        // Perform stretch IK.
        const float limbLength = (lengthA + lengthB) * m_stretchThreshold;
        if (limbLength < distToTarget && limbLength > 0.0f)
        {
            const float scale = AZ::GetMin(m_stretchFactorMax, distToTarget / limbLength);
            lengthA *= scale;
            //lengthB *= scale;
        }

        const float d = (distToTarget > MCore::Math::epsilon) ? MCore::Max<float>(0.0f, MCore::Min<float>(lengthA, (distToTarget + (lengthA * lengthA - lengthB * lengthB) / distToTarget) * 0.5f)) : MCore::Max<float>(0.0f, MCore::Min<float>(lengthA, distToTarget));
        const float square = lengthA * lengthA - d * d;
        const float e = MCore::Math::SafeSqrt(square);

        const AZ::Vector3 solution(d, e, 0);
        MCore::Matrix matForward;
        matForward.Identity();
        CalculateMatrix(localGoal, bendDir, &matForward);

        *outMidPos = posA + matForward.Mul3x3(solution);
        return (d > MCore::Math::epsilon && d < lengthA + MCore::Math::epsilon);
    }

    // Calculate the matrix to rotate the solve plane.
    void BlendTreeFootIKNode::CalculateMatrix(const AZ::Vector3& goal, const AZ::Vector3& bendDir, MCore::Matrix* outForward)
    {
        const AZ::Vector3 x = MCore::SafeNormalize(goal);
        const float dot = bendDir.Dot(x);
        const AZ::Vector3 y = MCore::SafeNormalize(bendDir - (dot * x));
        const AZ::Vector3 z = x.Cross(y);
        outForward->SetRow(0, x);
        outForward->SetRow(1, y);
        outForward->SetRow(2, z);
    }

    // Generate the ray start and end position.
    void BlendTreeFootIKNode::GenerateRayStartEnd(LegId legId, LegJointId jointId, AnimGraphInstance* animGraphInstance, UniqueData* uniqueData, const Pose& inputPose, AZ::Vector3& outRayStart, AZ::Vector3& outRayEnd) const
    {
        const AZ::u32 jointIndex = uniqueData->m_legs[legId].m_jointIndices[jointId];
        AZ_Assert(jointIndex != MCORE_INVALIDINDEX32, "Expecting the joint index to be valid.");

        const float rayLength = GetRaycastLength(animGraphInstance);
        const AZ::Vector3 upVector = animGraphInstance->GetActorInstance()->GetWorldSpaceTransform().mRotation * AZ::Vector3(0.0f, 0.0f, 1.0f);
        const AZ::Vector3 jointPositionModelSpace = inputPose.GetModelSpaceTransform(jointIndex).mPosition;
        const AZ::Vector3 hipPositionModelSpace = inputPose.GetModelSpaceTransform(uniqueData->m_hipJointIndex).mPosition;
        const AZ::Vector3 jointPositionWorldSpace = inputPose.GetWorldSpaceTransform(jointIndex).mPosition;
        const AZ::Vector3 hipPositionWorldSpace = inputPose.GetWorldSpaceTransform(uniqueData->m_hipJointIndex).mPosition;
        const float hipHeightDiff = hipPositionModelSpace.GetZ() - jointPositionModelSpace.GetZ();
        outRayStart = jointPositionWorldSpace + upVector * hipHeightDiff;
        outRayEnd = jointPositionWorldSpace - upVector * rayLength;
    }

    void BlendTreeFootIKNode::Raycast(LegId legId, LegJointId jointId, AnimGraphInstance* animGraphInstance, UniqueData* uniqueData, const Pose& inputPose, RaycastResult& raycastResult)
    {
        // Generate the ray start and end position.
        AZ::Vector3 rayStart;
        AZ::Vector3 rayEnd;
        GenerateRayStartEnd(legId, jointId, animGraphInstance, uniqueData, inputPose, rayStart, rayEnd);

        // Normalize the ray direction.
        AZ::Vector3 rayDirection = rayEnd - rayStart;
        const float maxDistance = rayDirection.GetLength();
        if (maxDistance > 0.0f)
        {
            rayDirection /= maxDistance;
        }

        // Scale the height offset by the actor instance scale.
        const float actorInstanceScale = animGraphInstance->GetActorInstance()->GetWorldSpaceTransform().mScale.GetZ();
        float heightOffset = GetFootHeightOffset(animGraphInstance);
        if (jointId == LegJointId::Foot)
        {
            heightOffset += uniqueData->m_legs[legId].m_footHeight * actorInstanceScale;
        }
        else
        {
            heightOffset += uniqueData->m_legs[legId].m_toeHeight * actorInstanceScale;
        }

        Integration::RaycastRequests::RaycastRequest rayRequest;
        rayRequest.m_start = rayStart;
        rayRequest.m_direction = rayDirection;
        rayRequest.m_distance = maxDistance;
        rayRequest.m_queryType = Physics::QueryType::Static;
        rayRequest.m_hint = Integration::RaycastRequests::UsecaseHint::FootPlant;

        // Cast a ray, check for intersections.
        Integration::RaycastRequests::RaycastResult rayResult;
        if (animGraphInstance->GetActorInstance()->GetIsOwnedByRuntime() || m_forceUseRaycastBus)
        {
            Integration::RaycastRequestBus::BroadcastResult(rayResult, &Integration::RaycastRequestBus::Events::Raycast, animGraphInstance->GetActorInstance()->GetEntityId(), rayRequest);
            if (rayResult.m_intersected)
            {
                ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
                raycastResult.m_position = rayResult.m_position + actorInstance->GetWorldSpaceTransform().mRotation * (AZ::Vector3(0.0f, 0.0f, heightOffset));
                raycastResult.m_normal = rayResult.m_normal;
                raycastResult.m_intersected = true;
            }
            else
            {
                raycastResult.m_position = AZ::Vector3::CreateZero();
                raycastResult.m_normal = AZ::Vector3(0.0f, 0.0f, 1.0f);
                raycastResult.m_intersected = false;
            }
        }
        else // If we are in the animation editor there is no environment, so just fake it with a ground plane. Don't use PhysX for raycasting there.
        {
            const AZ::Plane groundPlane = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.0f, 0.0f, 1.0f), AZ::Vector3::CreateZero());
            raycastResult.m_intersected = groundPlane.IntersectSegment(rayStart, rayEnd, raycastResult.m_position);
            raycastResult.m_normal.Set(0.0f, 0.0f, 1.0f);
            if (raycastResult.m_intersected)
            {
                raycastResult.m_position += AZ::Vector3(0.0f, 0.0f, heightOffset);
            }
        }

        // Draw the debug rays.
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            DebugDraw::ActorInstanceData* drawData = GetDebugDraw().GetActorInstanceData(animGraphInstance->GetActorInstance());
            drawData->Lock();
            if (raycastResult.m_intersected)
            {
                drawData->DrawLine(rayStart, raycastResult.m_position, AZ::Color(0.6f, 0.6f, 0.6f, 1.0f));
                drawData->DrawLine(raycastResult.m_position, rayEnd, AZ::Color(0.3f, 0.3f, 0.3f, 1.0f));
                drawData->DrawLine(raycastResult.m_position, raycastResult.m_position + raycastResult.m_normal * 0.1f, AZ::Color(1.0f, 1.0f, 0.0f, 1.0f));
            }
            else
            {
                drawData->DrawLine(rayStart, rayEnd, AZ::Color(0.3f, 0.3f, 0.3f, 1.0f));
            }
            drawData->Unlock();
        }
    }

    // Check whether IK should be active or not.
    float BlendTreeFootIKNode::CalculateIKWeightFactor(LegId legId, const IKSolveParameters& solveParams)
    {
        const Leg& leg = solveParams.m_uniqueData->m_legs[legId];
        if (leg.IsFlagEnabled(LegFlags::Locked))
        {
            return 1.0f;
        }
        return leg.IsFlagEnabled(LegFlags::FootDown) ? 1.0f : 0.0f;
    }

    // Smoothly interpolate the IK target weight towards the weight we want.
    void BlendTreeFootIKNode::InterpolateWeight(LegId legId, UniqueData* uniqueData, float timeDelta, float ikBlendSpeed)
    {
        Leg& leg = uniqueData->m_legs[legId];
        const float diff = fabs(leg.m_weight - leg.m_targetWeight);
        if (diff > 0.001f)
        {
            float t = timeDelta * ikBlendSpeed * s_ikSpeedMultiplier;
            if (t > 1.0)
            {
                t = 1.0;
            }

            leg.m_weight = AZ::Lerp(leg.m_weight, leg.m_targetWeight, t);
        }
        else
        {
            leg.m_weight = leg.m_targetWeight;
        }
    }

    // Calculate the rotation of the feet when it would be aligned to the surface below.
    AZ::Quaternion BlendTreeFootIKNode::CalculateFootRotation(LegId legId, const IKSolveParameters& solveParams) const
    {
        const Leg& leg = solveParams.m_uniqueData->m_legs[legId];

        AZ::Quaternion result = MCore::EmfxQuatToAzQuat(solveParams.m_outputPose->GetWorldSpaceTransform(leg.m_jointIndices[LegJointId::Foot]).mRotation);
        const bool footDown = leg.IsFlagEnabled(LegFlags::FootDown);
        const bool toeDown = leg.IsFlagEnabled(LegFlags::ToeDown);
        const float weight = leg.m_weight * solveParams.m_weight;
        if (!solveParams.m_forceIKDisabled && leg.IsFlagEnabled(LegFlags::IkEnabled) && weight > AZ::g_fltEps)
        {
            const AZ::u32 footIndex = leg.m_jointIndices[LegJointId::Foot];
            const AZ::u32 toeIndex = leg.m_jointIndices[LegJointId::Toe];

            // When both foot and toe are on the floor
            float distToToeTarget = 0.01f;
            if (solveParams.m_intersections[legId].m_toeResult.m_intersected)
            {
                distToToeTarget = (solveParams.m_outputPose->GetWorldSpaceTransform(footIndex).mPosition - solveParams.m_intersections[legId].m_toeResult.m_position).GetLength();
            }

            bool bothPlanted = false;
            if (footDown && toeDown && (distToToeTarget <= leg.m_footLength))
            {
                bothPlanted = true;

                // Get the current vector from the foot to the toe.
                const AZ::Vector3 footPos = solveParams.m_outputPose->GetWorldSpaceTransform(footIndex).mPosition;
                const AZ::Vector3 oldToePos = solveParams.m_outputPose->GetWorldSpaceTransform(toeIndex).mPosition;
                const AZ::Vector3 oldToToe = (oldToePos - footPos).GetNormalizedSafeExact();

                // Get the new vector from the foot to the toe.
                const AZ::Vector3& newToePos = solveParams.m_intersections[legId].m_toeResult.m_position;
                const AZ::Vector3 newToToe = (newToePos - footPos).GetNormalizedSafeExact();

                // Apply a delta rotation to the foot.
                Transform newTransform = solveParams.m_outputPose->GetWorldSpaceTransform(footIndex);
                const AZ::Quaternion deltaRot = AZ::Quaternion::CreateShortestArc(oldToToe, newToToe);
                result = deltaRot * MCore::EmfxQuatToAzQuat(newTransform.mRotation);
            }
            else if (footDown)
            {
                // Get the current vector from the foot to the toe.
                const AZ::Vector3 footPos = solveParams.m_outputPose->GetWorldSpaceTransform(footIndex).mPosition;
                const AZ::Vector3 oldToePos = solveParams.m_outputPose->GetWorldSpaceTransform(toeIndex).mPosition;
                const AZ::Vector3 oldToToe = (oldToePos - footPos).GetNormalizedSafeExact();

                // Get the new vector from the foot to the toe.
                const IntersectionResults& intersections = solveParams.m_intersections[legId];
                const float footToeHeightDiff = leg.m_footHeight - leg.m_toeHeight;
                const AZ::Plane plane = AZ::Plane::CreateFromNormalAndPoint(intersections.m_footResult.m_normal, intersections.m_footResult.m_position);
                const AZ::Vector3 offset = solveParams.m_actorInstance->GetWorldSpaceTransform().mRotation * (footToeHeightDiff * intersections.m_footResult.m_normal);
                AZ::Vector3 newToePos = plane.GetProjected(oldToToe);
                newToePos = intersections.m_footResult.m_position + newToePos.GetNormalizedSafeExact() * leg.m_footLength;
                newToePos -= offset;
                const AZ::Vector3 newToToe = (newToePos - footPos).GetNormalizedSafeExact();

                // Apply a delta rotation to the foot.
                Transform newTransform = solveParams.m_outputPose->GetWorldSpaceTransform(footIndex);
                const AZ::Quaternion deltaRot = AZ::Quaternion::CreateShortestArc(oldToToe, newToToe);
                result = deltaRot * MCore::EmfxQuatToAzQuat(newTransform.mRotation);
            }

            // Visualize some debug things in the viewport.
            if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(solveParams.m_animGraphInstance) && footDown)
            {
                const IntersectionResults& intersections = solveParams.m_intersections[legId];
                DebugDraw::ActorInstanceData* drawData = GetDebugDraw().GetActorInstanceData(solveParams.m_actorInstance);
                drawData->Lock();
                const float s = s_visualizeFootPlaneScale;
                const AZ::Vector3& p = intersections.m_footResult.m_position;
                const AZ::Plane plane = AZ::Plane::CreateFromNormalAndPoint(intersections.m_footResult.m_normal, intersections.m_footResult.m_position);
                const AZ::Color color = bothPlanted ? AZ::Color(0.0f, 1.0f, 0.0f, 1.0f) : AZ::Color(0.0f, 1.0f, 1.0f, 1.0f);

                drawData->DrawLine(p + plane.GetProjected(AZ::Vector3(-s, 0.0f, 0.0f)), p + plane.GetProjected(AZ::Vector3(s, 0.0f, 0.0f)), color);
                drawData->DrawLine(p + plane.GetProjected(AZ::Vector3(-s, -s, 0.0f)), p + plane.GetProjected(AZ::Vector3(-s, s, 0.0f)), color);
                drawData->DrawLine(p + plane.GetProjected(AZ::Vector3(s, -s, 0.0f)), p + plane.GetProjected(AZ::Vector3(s, s, 0.0f)), color);
                drawData->DrawLine(p + plane.GetProjected(AZ::Vector3(0.0f, -s, 0.0f)), p + plane.GetProjected(AZ::Vector3(0.0f, s, 0.0f)), color);
                drawData->DrawLine(p + plane.GetProjected(AZ::Vector3(-s, -s, 0.0f)), p + plane.GetProjected(AZ::Vector3(s, -s, 0.0f)), color);
                drawData->DrawLine(p + plane.GetProjected(AZ::Vector3(-s, s, 0.0f)), p + plane.GetProjected(AZ::Vector3(s, s, 0.0f)), color);
                if (leg.IsFlagEnabled(LegFlags::Locked) && solveParams.m_footLock && !leg.IsFlagEnabled(LegFlags::FirstUpdate))
                {
                    const float m = s * 0.5f;
                    drawData->DrawLine(p + plane.GetProjected(AZ::Vector3(-m, -m, 0.0f)), p + plane.GetProjected(AZ::Vector3(-m, m, 0.0f)), color);
                    drawData->DrawLine(p + plane.GetProjected(AZ::Vector3(m, -m, 0.0f)), p + plane.GetProjected(AZ::Vector3(m, m, 0.0f)), color);
                    drawData->DrawLine(p + plane.GetProjected(AZ::Vector3(-m, -m, 0.0f)), p + plane.GetProjected(AZ::Vector3(m, -m, 0.0f)), color);
                    drawData->DrawLine(p + plane.GetProjected(AZ::Vector3(-m, m, 0.0f)), p + plane.GetProjected(AZ::Vector3(m, m, 0.0f)), color);
                }
                drawData->Unlock();
            }
        }

        return result;
    }

    /*
    bool BlendTreeFootIKNode::IsFootBelowSurface(LegId legId, UniqueData* uniqueData, const AZ::Vector3& upperLegPosition, const AZ::Vector3& position, const AZ::Vector3& intersectionPoint, const AZ::Vector3& intersectionNormal, float threshold) const
    {
        AZ_UNUSED(position);
        const AZ::Plane plane = AZ::Plane::CreateFromNormalAndPoint(intersectionNormal, intersectionPoint);

        const float distToTarget = (upperLegPosition - intersectionPoint).GetLengthExact();
        const float legLength = uniqueData->m_legs[legId].m_legLength;

        const AZ::Vector3 furthestFootPos = upperLegPosition + (intersectionPoint - upperLegPosition).GetNormalizedSafeExact() * legLength * GetStretchMaxFactor();

        //return (plane.GetPointDist(position) <= threshold);
        return (plane.GetPointDist(furthestFootPos) <= threshold);
    }
*/

    bool BlendTreeFootIKNode::IsBelowSurface(const AZ::Vector3& position, const AZ::Vector3& intersectionPoint, const AZ::Vector3& intersectionNormal, float threshold) const
    {
        const AZ::Plane plane = AZ::Plane::CreateFromNormalAndPoint(intersectionNormal, intersectionPoint);
        return (plane.GetPointDist(position) <= threshold);
    }

    // Calculate the new transforms for a specific leg.
    void BlendTreeFootIKNode::SolveLegIK(LegId legId, const IKSolveParameters& solveParams)
    {
        Leg& leg = solveParams.m_uniqueData->m_legs[legId];
        const AZ::u32 upperLegIndex = leg.m_jointIndices[LegJointId::UpperLeg];
        const AZ::u32 kneeIndex = leg.m_jointIndices[LegJointId::Knee];
        const AZ::u32 footIndex = leg.m_jointIndices[LegJointId::Foot];

        // Calculate the world space transforms of the joints inside the leg.
        Transform inputGlobalTransforms[4];
        for (size_t legJointIndex = 0; legJointIndex < 4; ++legJointIndex)
        {
            inputGlobalTransforms[legJointIndex] = solveParams.m_inputPose->GetWorldSpaceTransform(leg.m_jointIndices[legJointIndex]);
        }

        // Get the target position for the foot and toe (the intersection points on the ground).
        AZ::Vector3 footTargetPosition = solveParams.m_intersections[legId].m_footResult.m_position;
        AZ::Vector3 toeTargetPosition = solveParams.m_intersections[legId].m_toeResult.m_position;

        const bool ikEnabled = leg.IsFlagEnabled(LegFlags::IkEnabled);

        // Check if we are below the surface or not.
        bool footDown;
        bool toeDown;
        bool justLocked = false;
        if (!solveParams.m_forceIKDisabled && ikEnabled)
        {
            const float actorInstanceScale = solveParams.m_actorInstance->GetWorldSpaceTransform().mScale.GetZ();
            const float surfaceOffset = s_surfaceThreshold * actorInstanceScale;
            footDown = solveParams.m_intersections[legId].m_footResult.m_intersected ? IsBelowSurface(inputGlobalTransforms[LegJointId::Foot].mPosition, footTargetPosition, solveParams.m_intersections[legId].m_footResult.m_normal, surfaceOffset) : false;
            toeDown = solveParams.m_intersections[legId].m_toeResult.m_intersected ? IsBelowSurface(inputGlobalTransforms[LegJointId::Toe].mPosition, toeTargetPosition, solveParams.m_intersections[legId].m_toeResult.m_normal, surfaceOffset) : false;
        }
        else
        {
            footDown = false;
            toeDown = false;
        }

        leg.SetFlag(LegFlags::FootDown, footDown);
        leg.SetFlag(LegFlags::ToeDown, toeDown);

        // Handle foot locking position.
        if (solveParams.m_footLock)
        {
            // If we are forced to start unlocking as it isn't allowed anymore by the events.
            if (leg.IsFlagEnabled(LegFlags::Locked) && !leg.IsFlagEnabled(LegFlags::AllowLocking))
            {
                leg.DisableFlag(LegFlags::Locked);
                leg.EnableFlag(LegFlags::Unlocking);
                leg.m_unlockBlendT = 0.0f;
            }

            // If we are in unlocked state, but our foot is fully planted, start locking the foot.
            if (!leg.IsFlagEnabled(LegFlags::Locked) && (footDown && toeDown) && leg.IsFlagEnabled(LegFlags::AllowLocking) && !leg.IsFlagEnabled(LegFlags::Unlocking))
            {
                leg.EnableFlag(LegFlags::Locked);
                leg.DisableFlag(LegFlags::Unlocking);
                leg.m_footLockPosition = footTargetPosition;
                justLocked = true;
            }

            // If we are in the process of unlocking, blend into the unlocked state.
            if (leg.IsFlagEnabled(LegFlags::Unlocking))
            {
                leg.m_unlockBlendT += solveParams.m_deltaTime * 4.0f;
                if (leg.m_unlockBlendT > 1.0f)
                {
                    leg.m_unlockBlendT = 1.0f;
                    leg.DisableFlag(LegFlags::Unlocking);
                }

                footTargetPosition = leg.m_footLockPosition.Lerp(footTargetPosition, leg.m_unlockBlendT);
            }
            else if (leg.IsFlagEnabled(LegFlags::Locked))
            {
                footTargetPosition = leg.m_footLockPosition;
            }
        }
        else
        {
            leg.DisableFlag(LegFlags::Locked);
            leg.DisableFlag(LegFlags::Unlocking);
            leg.m_unlockBlendT = 0.0f;
            leg.m_footLockPosition = footTargetPosition;
            leg.m_footLockRotation = AZ::Quaternion::CreateIdentity();
        }

        // Limit the target position in height.
        const AZ::Vector3 vecToTarget = solveParams.m_actorInstance->GetWorldSpaceTransformInversed().mRotation * (footTargetPosition - inputGlobalTransforms[LegJointId::Foot].mPosition);
        const float feetDifference = vecToTarget.GetZ();
        const float maxFootAdjustment = GetMaxFootAdjustment(solveParams.m_animGraphInstance);
        if (feetDifference > maxFootAdjustment)
        {
            return;
        }

        // Calculate the pole vector.
        const AZ::Vector3 toFoot = (inputGlobalTransforms[LegJointId::Foot].mPosition - inputGlobalTransforms[LegJointId::UpperLeg].mPosition).GetNormalizedSafeExact();
        AZ::Vector3 toKnee = (inputGlobalTransforms[LegJointId::Knee].mPosition - inputGlobalTransforms[LegJointId::UpperLeg].mPosition).GetNormalizedSafeExact();
        if (AZ::IsClose(toFoot.Dot(toKnee), 1.0f, 0.001f))
        {
            toKnee += solveParams.m_actorInstance->GetWorldSpaceTransform().mRotation * AZ::Vector3(0.0f, 0.01f, 0.0f);
            toKnee.NormalizeSafeExact();
        }
        const AZ::Vector3 planeNormal = toFoot.Cross(toKnee);
        const AZ::Vector3 finalPoleVector = planeNormal.Cross(toFoot);

        // Solve the two joint IK problem by calculating the new position of the knee.
        AZ::Vector3 kneePos;
        Solve2LinkIK(inputGlobalTransforms[LegJointId::UpperLeg].mPosition, inputGlobalTransforms[LegJointId::Knee].mPosition, inputGlobalTransforms[LegJointId::Foot].mPosition, footTargetPosition, finalPoleVector, &kneePos);

        // Update the upper leg.
        AZ::Vector3 oldForward = (inputGlobalTransforms[LegJointId::Knee].mPosition - inputGlobalTransforms[LegJointId::UpperLeg].mPosition).GetNormalizedSafeExact();
        AZ::Vector3 newForward = (kneePos - inputGlobalTransforms[LegJointId::UpperLeg].mPosition).GetNormalizedSafeExact();
        Transform newTransform = inputGlobalTransforms[LegJointId::UpperLeg];
        newTransform.mRotation.RotateFromTo(oldForward, newForward);
        solveParams.m_outputPose->SetWorldSpaceTransform(upperLegIndex, newTransform);

        // Update the knee.
        const AZ::Vector3 footPos = solveParams.m_outputPose->GetWorldSpaceTransform(leg.m_jointIndices[LegJointId::Foot]).mPosition;
        oldForward = (footPos - kneePos).GetNormalizedExact();
        newForward = (footTargetPosition - kneePos).GetNormalizedSafeExact();
        newTransform = solveParams.m_outputPose->GetWorldSpaceTransform(leg.m_jointIndices[LegJointId::Knee]);
        newTransform.mRotation.RotateFromTo(oldForward, newForward);
        newTransform.mPosition = kneePos;
        solveParams.m_outputPose->SetWorldSpaceTransform(kneeIndex, newTransform);

        if (leg.IsFlagEnabled(LegFlags::FirstUpdate))
        {
            leg.m_currentFootRot = MCore::EmfxQuatToAzQuat(inputGlobalTransforms[LegJointId::Foot].mRotation);
            leg.DisableFlag(LegFlags::FirstUpdate);
        }

        leg.m_targetWeight = CalculateIKWeightFactor(legId, solveParams);
        const float weight = leg.m_weight * solveParams.m_weight;

        AZ::Quaternion footRotation = CalculateFootRotation(legId, solveParams);
        Transform footTransform = solveParams.m_outputPose->GetWorldSpaceTransform(footIndex);

        // Handle foot lock rotation.
        if (solveParams.m_footLock && !solveParams.m_forceIKDisabled && leg.IsFlagEnabled(LegFlags::IkEnabled))
        {
            if (justLocked)
            {
                leg.m_footLockRotation = footRotation;
            }

            if (leg.IsFlagEnabled(LegFlags::Locked))
            {
                footRotation = leg.m_footLockRotation;
            }
        }

        float blendT = s_ikSpeedMultiplier * GetFootBlendSpeed(solveParams.m_animGraphInstance) * solveParams.m_deltaTime;
        if (blendT > 1.0f)
        {
            blendT = 1.0f;
        }
        leg.m_currentFootRot = leg.m_currentFootRot.NLerp(footRotation, blendT);
        footTransform.mRotation = MCore::AzQuatToEmfxQuat(leg.m_currentFootRot);
        solveParams.m_outputPose->SetWorldSpaceTransform(footIndex, footTransform);

        // Draw debug lines.
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(solveParams.m_animGraphInstance))
        {
            DebugDraw::ActorInstanceData* drawData = GetDebugDraw().GetActorInstanceData(solveParams.m_actorInstance);
            drawData->Lock();
            if (!solveParams.m_forceIKDisabled && leg.IsFlagEnabled(LegFlags::IkEnabled) && solveParams.m_intersections[legId].m_footResult.m_intersected)
            {
                drawData->DrawLine(inputGlobalTransforms[LegJointId::UpperLeg].mPosition, kneePos, mVisualizeColor);
                drawData->DrawLine(kneePos, footTargetPosition, mVisualizeColor);
            }
            drawData->Unlock();
        }

        // Blend using the IK weight for the nodes where we didn't take this into account yet.
        // This is basically the knee and upper leg.
        if (weight < 0.9999f)
        {
            for (size_t i = 1; i < 4; ++i)
            {
                const AZ::u32 nodeIndex = leg.m_jointIndices[Toe - i];
                solveParams.m_outputPose->UpdateLocalSpaceTransform(nodeIndex);
                Transform finalTransform = solveParams.m_inputPose->GetLocalSpaceTransform(nodeIndex);
                finalTransform.Blend(solveParams.m_outputPose->GetLocalSpaceTransform(nodeIndex), weight);
                solveParams.m_outputPose->SetLocalSpaceTransform(nodeIndex, finalTransform);
            }
        }
    }

    // Update the length of a leg and its foot.
    void BlendTreeFootIKNode::UpdateLegLength(LegId legId, UniqueData* uniqueData, const Pose& inputPose)
    {
        Leg& leg = uniqueData->m_legs[legId];

        // Calculate the leg length.
        leg.m_legLength = 0.0f;
        for (size_t legNodeIndex = 1; legNodeIndex < 3; ++legNodeIndex)
        {
            leg.m_legLength += (inputPose.GetModelSpaceTransform(leg.m_jointIndices[legNodeIndex]).mPosition - inputPose.GetModelSpaceTransform(leg.m_jointIndices[legNodeIndex - 1]).mPosition).GetLengthExact();
        }

        // Calculate the foot length.
        leg.m_footLength = (inputPose.GetModelSpaceTransform(leg.m_jointIndices[LegJointId::Toe]).mPosition - inputPose.GetModelSpaceTransform(leg.m_jointIndices[LegJointId::Foot]).mPosition).GetLengthExact();
    }

    // Adjust the hip by moving it downwards when we can't reach a given target.
    float BlendTreeFootIKNode::AdjustHip(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData, Pose& inputPose, Pose& outputPose, IntersectionResults* intersectionResults, bool allowAdjust)
    {
        float correction = 0.0f;

        // If both our legs have an intersection target.
        if (intersectionResults[LegId::Left].m_footResult.m_intersected && intersectionResults[LegId::Right].m_footResult.m_intersected && allowAdjust)
        {
            const Leg& leftLeg = uniqueData->m_legs[LegId::Left];
            const Leg& rightLeg = uniqueData->m_legs[LegId::Right];

            // If the target foot position is below the ground plane in model space, so if we actually have to lower the hips.
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            const AZ::Vector3 upVector = actorInstance->GetWorldSpaceTransform().mRotation * AZ::Vector3(0.0f, 0.0f, 1.0f);
            const AZ::Vector3 leftFootBindPoseModelSpace = actorInstance->GetTransformData()->GetBindPose()->GetModelSpaceTransform(leftLeg.m_jointIndices[LegJointId::Foot]).mPosition;
            const AZ::Vector3 leftFootBindWorldPos = actorInstance->GetWorldSpaceTransform().TransformPoint(leftFootBindPoseModelSpace);
            const AZ::Plane leftSurfacePlane = AZ::Plane::CreateFromNormalAndPoint(upVector, leftFootBindWorldPos);
            float leftCorrection = leftSurfacePlane.GetPointDist(intersectionResults[LegId::Left].m_footResult.m_position);
            if (leftCorrection > 0.0f)
            {
                leftCorrection = 0.0f;
            }

            // Do the same for the right leg.
            const AZ::Vector3 rightFootBindPoseModelSpace = actorInstance->GetTransformData()->GetBindPose()->GetModelSpaceTransform(rightLeg.m_jointIndices[LegJointId::Foot]).mPosition;
            const AZ::Vector3 rightFootBindWorldPos = actorInstance->GetWorldSpaceTransform().TransformPoint(rightFootBindPoseModelSpace);
            const AZ::Plane rightSurfacePlane = AZ::Plane::CreateFromNormalAndPoint(upVector, rightFootBindWorldPos);
            float rightCorrection = rightSurfacePlane.GetPointDist(intersectionResults[LegId::Right].m_footResult.m_position);
            if (rightCorrection > 0.0f)
            {
                rightCorrection = 0.0f;
            }

            // Get the maximum required downward movement.
            const float maxDist = AZ::GetMin(leftCorrection, rightCorrection);
            const float maxHipAdjustment = GetMaxHipAdjustment(animGraphInstance);
            correction = maxDist;
            correction = AZ::GetClamp(correction, -maxHipAdjustment, 0.0f);

            // Debug render some line to show the displacement.
            if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
            {
                const AZ::Vector3 hipPos = inputPose.GetWorldSpaceTransform(uniqueData->m_hipJointIndex).mPosition;
                DebugDraw::ActorInstanceData* drawData = GetDebugDraw().GetActorInstanceData(animGraphInstance->GetActorInstance());
                drawData->Lock();
                drawData->DrawLine(hipPos, hipPos + AZ::Vector3(0.0f, 0.0f, correction), AZ::Color(1.0f, 0.0f, 1.0f, 1.0f));
                drawData->Unlock();
            }
        }

        // Perform the actual hip adjustment.
        Transform hipTransform = outputPose.GetWorldSpaceTransform(uniqueData->m_hipJointIndex);
        uniqueData->m_hipCorrectionTarget = correction;
        float t = s_ikSpeedMultiplier * uniqueData->m_timeDelta * GetHipBlendSpeed(animGraphInstance);
        if (t > 1.0f)
        {
            t = 1.0f;
        }
        const float interpolatedCorrection = AZ::Lerp(uniqueData->m_curHipCorrection, correction, t);
        uniqueData->m_curHipCorrection = interpolatedCorrection;
        hipTransform.mPosition += animGraphInstance->GetActorInstance()->GetWorldSpaceTransform().mRotation * AZ::Vector3(0.0f, 0.0f, interpolatedCorrection);
        outputPose.SetWorldSpaceTransform(uniqueData->m_hipJointIndex, hipTransform);
        inputPose = outputPose; // As we adjusted our hip, the input pose to the IK leg solve has been modified, so update it.

        return correction;
    }

    // Output events and motion extraction.
    void BlendTreeFootIKNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AnimGraphNode::PostUpdate(animGraphInstance, timePassedInSeconds);

        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        if (data)
        {
            uniqueData->m_eventBuffer = data->GetEventBuffer();
        }
    }

    // Update the IK weights.
    void BlendTreeFootIKNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AnimGraphNode::Update(animGraphInstance, timePassedInSeconds);

        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        if (uniqueData)
        {
            uniqueData->m_timeDelta = timePassedInSeconds;
        }

        if (uniqueData && uniqueData->m_isValid)
        {
            const float ikBlendSpeed = GetIKBlendSpeed(animGraphInstance);
            InterpolateWeight(LegId::Left, uniqueData, timePassedInSeconds, ikBlendSpeed);
            InterpolateWeight(LegId::Right, uniqueData, timePassedInSeconds, ikBlendSpeed);
        }
    }

    // The main output function to calculate the joint transforms.
    void BlendTreeFootIKNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphPose* outputPose;

        // If nothing is connected to the input pose, output a bind pose.
        if (!GetInputPort(INPUTPORT_POSE).mConnection)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            outputPose->InitFromBindPose(actorInstance);
            return;
        }

        // Get the weight from the input port.
        float weight = 1.0f;
        if (GetInputPort(INPUTPORT_WEIGHT).mConnection)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_WEIGHT));
            weight = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_WEIGHT);
            weight = MCore::Clamp<float>(weight, 0.0f, 1.0f);
        }

        // If the weight is near zero or if this node is disabled, we can skip all calculations and just output the input pose.
        if (weight < MCore::Math::epsilon || mDisabled)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
            const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *inputPose;
            return;
        }

        OutputAllIncomingNodes(animGraphInstance);

        // Get the input pose and copy it over to the output pose
        AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        *outputPose = *inputPose;

        // Check if we have a valid configuration.
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        UpdateUniqueData(animGraphInstance, uniqueData);
        if (!uniqueData->m_isValid)
        {
            if (GetEMotionFX().GetIsInEditorMode())
            {
                SetHasError(animGraphInstance, true);
            }
            return;
        }

        if (GetEMotionFX().GetIsInEditorMode())
        {
            SetHasError(animGraphInstance, false);
        }

        //-----------------------------------
        // Cast rays to find the height at the location of the feet and toes.
        IntersectionResults intersectionResults[2];
        Raycast(LegId::Left, LegJointId::Foot, animGraphInstance, uniqueData, inputPose->GetPose(), intersectionResults[LegId::Left].m_footResult);
        Raycast(LegId::Left, LegJointId::Toe, animGraphInstance, uniqueData, inputPose->GetPose(), intersectionResults[LegId::Left].m_toeResult);
        Raycast(LegId::Right, LegJointId::Foot, animGraphInstance, uniqueData, inputPose->GetPose(), intersectionResults[LegId::Right].m_footResult);
        Raycast(LegId::Right, LegJointId::Toe, animGraphInstance, uniqueData, inputPose->GetPose(), intersectionResults[LegId::Right].m_toeResult);

        // Calculate the leg lengths. Because we can scale the actor instances or bones, we have to recalculate this.
        // Checking whether scale changed is likely slower than just calculating the leg lengths, so we don't do such checks.
        UpdateLegLength(LegId::Left, uniqueData, inputPose->GetPose());
        UpdateLegLength(LegId::Right, uniqueData, inputPose->GetPose());

        // Reset some flags.
        uniqueData->m_legs[LegId::Left].DisableFlag(LegFlags::FootDown);
        uniqueData->m_legs[LegId::Right].DisableFlag(LegFlags::FootDown);

        uniqueData->m_legs[LegId::Left].DisableFlag(LegFlags::ToeDown);
        uniqueData->m_legs[LegId::Right].DisableFlag(LegFlags::ToeDown);

        uniqueData->m_legs[LegId::Left].DisableFlag(LegFlags::IkEnabled);
        uniqueData->m_legs[LegId::Right].DisableFlag(LegFlags::IkEnabled);

        uniqueData->m_legs[LegId::Left].DisableFlag(LegFlags::AllowLocking);
        uniqueData->m_legs[LegId::Right].DisableFlag(LegFlags::AllowLocking);

        // Try to figure out based on our events, whether IK should be active or not and if we should lock the feet in place or not.
        bool isLocked[2]{ false, false };
        bool ikDisabled[2];

        if (m_footPlantMethod == FootPlantDetectionMethod::Automatic)
        {
            ikDisabled[LegId::Left] = false;
            ikDisabled[LegId::Right] = false;
        }
        else
        {
            ikDisabled[LegId::Left] = true;
            ikDisabled[LegId::Right] = true;
        }

        const AnimGraphEventBuffer& eventBuffer = uniqueData->m_eventBuffer;
        const AZ::u32 numEvents = eventBuffer.GetNumEvents();
        for (AZ::u32 i = 0; i < numEvents; ++i)
        {
            const EventInfo& eventInfo = eventBuffer.GetEvent(i);
            const MotionEvent* motionEvent = eventInfo.mEvent;
            const EventDataSet& eventDataSet = motionEvent->GetEventDatas();
            for (const EventDataPtr& eventData : eventDataSet)
            {
                if (azrtti_istypeof<EventDataFootIK>(eventData.get()))
                {
                    const EventDataFootIK* ikEvent = static_cast<const EventDataFootIK*>(eventData.get());
                    const bool locked = ikEvent->GetLocked();

                    // We're dealing with the left foot.
                    if (ikEvent->GetFoot() == EventDataFootIK::Foot::Left)
                    {
                        if (locked)
                        {
                            isLocked[LegId::Left] = true;
                        }

                        if (m_footPlantMethod != FootPlantDetectionMethod::Automatic)
                        {
                            if (ikEvent->GetIKEnabled())
                            {
                                ikDisabled[LegId::Left] = false;
                            }
                        }
                        else
                        {
                            if (!ikEvent->GetIKEnabled())
                            {
                                ikDisabled[LegId::Left] = true;
                            }
                        }
                    }
                    else if (ikEvent->GetFoot() == EventDataFootIK::Foot::Right) // We're dealing with the right foot.
                    {
                        if (locked)
                        {
                            isLocked[LegId::Right] = true;
                        }

                        if (m_footPlantMethod != FootPlantDetectionMethod::Automatic)
                        {
                            if (ikEvent->GetIKEnabled())
                            {
                                ikDisabled[LegId::Right] = false;
                            }
                        }
                        else
                        {
                            if (!ikEvent->GetIKEnabled())
                            {
                                ikDisabled[LegId::Right] = true;
                            }
                        }
                    }
                    else // We're dealing with both feet.
                    {
                        AZ_Assert(ikEvent->GetFoot() == EventDataFootIK::Foot::Both, "Expected both feet to be down in the IK event foot type.");
                        if (locked)
                        {
                            isLocked[LegId::Left] = true;
                            isLocked[LegId::Right] = true;
                        }

                        if (m_footPlantMethod != FootPlantDetectionMethod::Automatic)
                        {
                            if (ikEvent->GetIKEnabled())
                            {
                                ikDisabled[LegId::Left] = false;
                                ikDisabled[LegId::Right] = false;
                            }
                        }
                        else
                        {
                            if (!ikEvent->GetIKEnabled())
                            {
                                ikDisabled[LegId::Left] = true;
                                ikDisabled[LegId::Right] = true;
                            }
                        }
                    }
                } // if we're a foot IK event
            } // for all event datas
        } // for all events

        uniqueData->m_legs[LegId::Left].SetFlag(LegFlags::IkEnabled, !ikDisabled[LegId::Left]);
        uniqueData->m_legs[LegId::Right].SetFlag(LegFlags::IkEnabled, !ikDisabled[LegId::Right]);

        // When this is set to true we will try to keep the feet locked whenever they hit the surface.
        uniqueData->m_legs[LegId::Left].SetFlag(LegFlags::AllowLocking, isLocked[LegId::Left]);
        uniqueData->m_legs[LegId::Right].SetFlag(LegFlags::AllowLocking, isLocked[LegId::Right]);

        // Adjust the hip position by moving it up or down if that would result in a more natural look.
        float hipHeightAdjustment = 0.0f;
        if (GetAdjustHip(animGraphInstance) && uniqueData->m_hipJointIndex != MCORE_INVALIDINDEX32)
        {
            hipHeightAdjustment = AdjustHip(animGraphInstance, uniqueData, inputPose->GetPose(), outputPose->GetPose(), intersectionResults, true /* allowHipAdjust */);
        }

        // Perform the leg IK.
        IKSolveParameters solveParams;
        solveParams.m_animGraphInstance = animGraphInstance;
        solveParams.m_actorInstance = animGraphInstance->GetActorInstance();
        solveParams.m_uniqueData = uniqueData;
        solveParams.m_inputPose = &inputPose->GetPose();
        solveParams.m_outputPose = &outputPose->GetPose();
        solveParams.m_intersections = intersectionResults;
        solveParams.m_weight = weight;
        solveParams.m_hipHeightAdj = hipHeightAdjustment;
        solveParams.m_deltaTime = uniqueData->m_timeDelta;
        solveParams.m_forceIKDisabled = false;
        solveParams.m_footLock = GetFootLock(animGraphInstance);
        SolveLegIK(LegId::Left, solveParams);
        SolveLegIK(LegId::Right, solveParams);
    }

    void BlendTreeFootIKNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNode::Rewind(animGraphInstance);

        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        Leg& leftLeg = uniqueData->m_legs[LegId::Left];
        leftLeg.m_flags = static_cast<AZ::u8>(LegFlags::FirstUpdate);
        leftLeg.m_weight = 0.0f;
        leftLeg.m_targetWeight = 0.0f;
        leftLeg.m_unlockBlendT = 0.0f;

        Leg& rightLeg = uniqueData->m_legs[LegId::Right];
        rightLeg.m_flags = static_cast<AZ::u8>(LegFlags::FirstUpdate);
        rightLeg.m_weight = 0.0f;
        rightLeg.m_targetWeight = 0.0f;
        rightLeg.m_unlockBlendT = 0.0f;
    }

    // update the parameter contents, such as combobox values.
    void BlendTreeFootIKNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // find our unique data
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (!uniqueData)
        {
            uniqueData = aznew UniqueData(this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        uniqueData->m_mustUpdate = true;
        UpdateUniqueData(animGraphInstance, uniqueData);
    }

    void BlendTreeFootIKNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeFootIKNode, AnimGraphNode>()
            ->Version(1)
            ->Field("leftFootJointName", &BlendTreeFootIKNode::m_leftFootJointName)
            ->Field("leftToeJointName", &BlendTreeFootIKNode::m_leftToeJointName)
            ->Field("rightFootJointName", &BlendTreeFootIKNode::m_rightFootJointName)
            ->Field("rightToeJointName", &BlendTreeFootIKNode::m_rightToeJointName)
            ->Field("hipJointName", &BlendTreeFootIKNode::m_hipJointName)
            ->Field("footPlantMethod", &BlendTreeFootIKNode::m_footPlantMethod)
            ->Field("raycastLength", &BlendTreeFootIKNode::m_raycastLength)
            ->Field("feetHeightOffset", &BlendTreeFootIKNode::m_footHeightOffset)
            ->Field("maxHipAdjustment", &BlendTreeFootIKNode::m_maxHipAdjustment)
            ->Field("maxFootAdjustment", &BlendTreeFootIKNode::m_maxFootAdjustment)
            ->Field("blendSpeed", &BlendTreeFootIKNode::m_ikBlendSpeed)
            ->Field("footBlendSpeed", &BlendTreeFootIKNode::m_footBlendSpeed)
            ->Field("hipBlendSpeed", &BlendTreeFootIKNode::m_hipBlendSpeed)
            ->Field("adjustHip", &BlendTreeFootIKNode::m_adjustHip)
            ->Field("footLock", &BlendTreeFootIKNode::m_footLock);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        const AZ::VectorFloat maxVecFloat = AZ::VectorFloat(std::numeric_limits<float>::max());
        auto root = editContext->Class<BlendTreeFootIKNode>("Footplant IK", "Footplant IK settings");
        root->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

        root->ClassElement(AZ::Edit::ClassElements::Group, "General settings")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->DataElement(AZ_CRC("ActorNode", 0x35d9eb50), &BlendTreeFootIKNode::m_leftFootJointName, "Left foot joint", "The left foot joint.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeFootIKNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC("ActorNode", 0x35d9eb50), &BlendTreeFootIKNode::m_leftToeJointName, "Left toe joint", "The left toe joint.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeFootIKNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC("ActorNode", 0x35d9eb50), &BlendTreeFootIKNode::m_rightFootJointName, "Right foot joint", "The right foot joint.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeFootIKNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC("ActorNode", 0x35d9eb50), &BlendTreeFootIKNode::m_rightToeJointName, "Right toe joint", "The right toe joint.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeFootIKNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC("ActorNode", 0x35d9eb50), &BlendTreeFootIKNode::m_hipJointName, "Hip joint", "The hip/pelvis joint. This join will be moved downward in some cases to make the feet reach the surface below.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeFootIKNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeFootIKNode::m_footPlantMethod, "Footplant method", "The detection method for foot planting.")
            ->EnumAttribute(FootPlantDetectionMethod::Automatic, "Automatic")
            ->EnumAttribute(FootPlantDetectionMethod::MotionEvents, "FootIK motion events")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeFootIKNode::m_raycastLength, "Raycast length", "The maximum distance from the hips towards the feet.")
            ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
            ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeFootIKNode::m_ikBlendSpeed, "Blend speed", "How fast should the leg IK blend in or out?")
            ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
            ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
            ->Attribute(AZ::Edit::Attributes::Step, 0.01f);

        root->ClassElement(AZ::Edit::ClassElements::Group, "Foot settings")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeFootIKNode::m_footHeightOffset, "Height offset", "The foot height offset, used to move the feet up or down, to align nicely to the surface.")
            ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
            ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeFootIKNode::m_footBlendSpeed, "Blend speed", "How fast should the foot alignment blend in or out?")
            ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
            ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
            ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeFootIKNode::m_maxFootAdjustment, "Max adjustment", "Disable the IK solve for the leg when the foot IK target would be further away than this number of units.")
            ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
            ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeFootIKNode::m_footLock, "Enable locking", "Enable foot locking? This locks the feet into fixed positions. Foot locking requires the use of motion events using the EventDataFootIK event data type.");

        root->ClassElement(AZ::Edit::ClassElements::Group, "Hip settings")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeFootIKNode::m_maxHipAdjustment, "Max adjustment", "The maximum number of units the hip can move when adjust hip is enabled.")
            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
            ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeFootIKNode::m_hipBlendSpeed, "Blend speed", "How fast should the hip changes blend?")
            ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
            ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
            ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeFootIKNode::m_adjustHip, "Enable adjustments", "Allow hip height adjustments?");
    }
} // namespace EMotionFX