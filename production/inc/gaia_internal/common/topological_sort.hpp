/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

/**
 * topological_sort.hpp
 *
 * Implements topological sorting (ordering) with arbitrary data.
 *
 * Licensed under the MIT License:
 *
 * Copyright (c) 2021 Cemalettin Dervis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include <algorithm>
#include <memory>
#include <vector>
#include <unordered_set>

namespace toposort
{
/**
 * Represents a graph that should be sorted.
 *
 * @param T The type of data a node inside the graph should store.
 */
template <typename T> class graph
{
public:
  class node;

  template <typename TN> using shared_ptr_t        = std::shared_ptr<TN>;
  template <typename TN> using vector_t            = std::vector<TN>;

  using node_ptr  = shared_ptr_t<node>;
  using node_list = vector_t<node_ptr>;

  /**
   * Represents a node inside a graph.
   * A node stores arbitrary data and a list of its dependencies.
   */
  class node final
  {
    friend class graph<T>;

  public:
    node(T&& data)
        : m_Data(std::forward<T>(data))
    {
    }

    node(const graph&) = delete;

    void operator=(const graph&) = delete;

    node(graph&&) = delete;

    void operator=(graph&&) = delete;

    ~node() noexcept = default;

    /**
     * Declares that this node depends on another node.
     *
     * @param node The node that this node depends on.
     */
    void depends_on(node_ptr node)
    {
      m_Children.push_back(std::move(node));
    }

    /**
     * Gets the node's stored data.
     */
    const T& data() const
    {
      return m_Data;
    }

  private:
    T         m_Data{};
    node_list m_Children;
  };

  /**
   * Defines the result of a sorting operation.
   */
  class result
  {
  public:
    explicit operator bool() const
    {
      return !sorted_nodes.empty();
    }

    node_list sorted_nodes;

    // If the graph contains a cyclic dependency, this is the first node in
    // question.
    node_ptr cyclic_a;

    // If the graph contains a cyclic dependency, this is the second node in
    // question.
    node_ptr cyclic_b;
  };

  /**
   * Clears the graph of all created nodes.
   */
  void clear()
  {
    m_Nodes.clear();
    m_UniqueNodes.clear();
  }

  /**
   * Creates a node that stores a specific value and does not have any
   * dependencies. Dependencies can be declared using the depends_on() method on
   * the returned node.
   *
   * @param value The value that should be stored by the node.
   */
  node_ptr create_node(T&& value)
  {
    if(m_UniqueNodes.count(value))
    {
      return nullptr;
    }
    m_Nodes.push_back(std::make_shared<node>(std::forward<T>(value)));
    m_UniqueNodes.insert(value);
    return m_Nodes.back();
  }

  /**
   * Sorts the graph and returns the results.
   */
  result sort()
  {
    result res{};

    if (!m_Nodes.empty())
    {
      node_ref_list dead;
      dead.reserve(m_Nodes.size());

      node_ref_list pending;
      pending.reserve(2u);

      if (!visit(m_Nodes, dead, pending, res))
        res = {};

      clear();
    }

    return res;
  }

private:
  // For temporary lists, we can use raw pointers instead of copying
  // around shared pointers.
  using node_ref_list = vector_t<const node*>;

  static bool visit(node_list& graph, node_ref_list& dead,
                    node_ref_list& pending, result& res)
  {
    for (const auto& n : graph)
    {
      const auto getNIterator = [&n](const node_ref_list& list) {
        return std::find_if(list.cbegin(), list.cend(),
                            [&n](const node* e) { return e == n.get(); });
      };

      const auto containsN = [&getNIterator](const node_ref_list& list) {
        return getNIterator(list) != list.cend();
      };

      if (containsN(dead))
        continue;

      if (containsN(pending))
      {
        const auto itPtr = std::find_if(graph.cbegin(), graph.cend(),
                                        [&pending](const node_ptr& e) {
                                          return e.get() == pending.back();
                                        });

        res.cyclic_a = n;
        res.cyclic_b = *itPtr;

        return false;
      }
      else
      {
        pending.push_back(n.get());
      }

      if (!visit(n->m_Children, dead, pending, res))
        return false;

      const auto it = getNIterator(pending);

      if (it != pending.end())
        pending.erase(it);

      dead.push_back(n.get());
      res.sorted_nodes.push_back(n);
    }

    return true;
  }

  node_list m_Nodes;
};
} // namespace toposort
