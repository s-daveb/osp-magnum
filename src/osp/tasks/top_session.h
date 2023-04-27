/**
 * Open Space Program
 * Copyright © 2019-2022 Open Space Program Project
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once

#include "builder.h"
#include "execute.h"
#include "tasks.h"
#include "top_tasks.h"
#include "top_utils.h"

#include <entt/core/any.hpp>

#include <cassert>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

#include <osp/unpack.h>

#define OSP_AUX_DCDI_C(session, topData, count, ...) \
    session.m_data.resize(count); \
    top_reserve(topData, 0, session.m_data.begin(), session.m_data.end()); \
    auto const [__VA_ARGS__] = osp::unpack<count>(session.m_data)
#define OSP_AUX_DCDI_B(x) x
#define OSP_AUX_DCDI_A(...) OSP_AUX_DCDI_B(OSP_AUX_DCDI_C(__VA_ARGS__))

/**
 * @brief
 */
#define OSP_DECLARE_CREATE_DATA_IDS(session, topData, arglist) OSP_AUX_DCDI_A(session, topData, arglist);


#define OSP_AUX_DGDI_C(session, count, ...) \
    auto const [__VA_ARGS__] = osp::unpack<count>(session.m_data)
#define OSP_AUX_DGDI_B(x) x
#define OSP_AUX_DGDI_A(...) OSP_AUX_DGDI_B(OSP_AUX_DGDI_C(__VA_ARGS__))

#define OSP_DECLARE_GET_DATA_IDS(session, arglist) OSP_AUX_DGDI_A(session, arglist);



namespace osp
{

/**
 * @brief A convenient group of TopData, Tasks, and Tags that work together to
 *        support a certain feature.
 *
 * Sessions only store vectors of integer IDs, and don't does not handle
 * ownership on its own. Close using osp::top_close_session before destruction.
 */
struct Session
{
    template <std::size_t N>
    std::array<TopDataId, N> acquire_data(ArrayView<entt::any> topData)
    {
        std::array<TopDataId, N> out;
        top_reserve(topData, 0, std::begin(out), std::end(out));
        m_data.assign(std::begin(out), std::end(out));
        return out;
    }

    template<typename TGT_STRUCT_T, typename BUILDER_T>
    TGT_STRUCT_T create_targets(BUILDER_T &rBuilder)
    {
        static_assert(sizeof(TGT_STRUCT_T) % sizeof(TargetId) == 0);
        constexpr std::size_t count = sizeof(TGT_STRUCT_T) / sizeof(TargetId);

        std::type_info const& info = typeid(TGT_STRUCT_T);
        m_targetStructHash = info.hash_code();
        m_targetStructName = info.name();

        m_targets.resize(count);

        rBuilder.m_rTasks.m_targetIds.create(m_targets.begin(), m_targets.end());

        return reinterpret_cast<TGT_STRUCT_T&>(*m_targets.data());
    }

    template<typename TGT_STRUCT_T>
    TGT_STRUCT_T get_targets() const
    {
        static_assert(sizeof(TGT_STRUCT_T) % sizeof(TargetId) == 0);
        constexpr std::size_t count = sizeof(TGT_STRUCT_T) / sizeof(TargetId);

        std::type_info const& info = typeid(TGT_STRUCT_T);
        LGRN_ASSERTMV(m_targetStructHash == info.hash_code() && count == m_targets.size(),
                      "get_targets must use the same struct given to create_targets",
                      m_targetStructHash, m_targetStructName,
                      info.hash_code(), info.name(),
                      m_targets.size());

        return reinterpret_cast<TGT_STRUCT_T const&>(*m_targets.data());
    }

    TaskId& task()
    {
        return m_tasks.emplace_back(lgrn::id_null<TaskId>());
    }

    std::vector<TopDataId>  m_data;
    std::vector<TargetId>   m_targets;
    std::vector<TaskId>     m_tasks;

    TargetId m_cleanupTgt{lgrn::id_null<TargetId>()};

    std::size_t m_targetStructHash{0};
    std::string m_targetStructName;
};

struct SessionGroup
{
    std::vector<Session> m_sessions;
    TaskEdges m_edges;
};

/**
 * @brief Close sessions, delete all their associated TopData, Tasks, and Targets.
 */
void top_close_session(Tasks& rTasks, ExecGraph const& graph, TopTaskDataVec_t& rTaskData, ArrayView<entt::any> topData, ExecContext& rExec, ArrayView<Session> sessions);


} // namespace osp
