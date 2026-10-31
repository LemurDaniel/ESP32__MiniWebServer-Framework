// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// NOTE:
//   Implementation of utility.path.h.

//
//   Chapters:
//     HELPERS         Splits a path into a segment stack.
//     PATH NODE       Insert and resolve in the trie.
//     RESOLVE PATHS   The PathResolver wrapper.
//

#include <algorithm>

#include <utils/utility.path.h>

namespace EspWeb
{
    // ===========================================================================
    // ===============================   HELPERS   ===============================
    // ===========================================================================
    //
    //   toSegmentStack   Splits a path into segments, the first segment last.
    //   appendHandlers   Appends one handler chain to another.
    //

    namespace
    {
        // The recursion consumes segments from the back, so the first segment goes last:
        // "GET/users/:id" => ["GET", "users", ":id"] => [":id", "users", "GET"]
        std::vector<std::string> toSegmentStack(const std::string &path)
        {
            std::vector<std::string> segments = fileHandler.split(path, "/");
            std::reverse(segments.begin(), segments.end());
            return segments;
        }

        void appendHandlers(std::vector<RequestHandler> &target, const std::vector<RequestHandler> &source)
        {
            target.insert(target.end(), source.begin(), source.end());
        }
    }

    // ===========================================================================
    // ==============================   PATH NODE   ==============================
    // ===========================================================================
    //
    //   add       Walks or creates one node per segment, stores the handlers at the end.
    //   resolve   Walks the matching nodes, literal before parameter.
    //

    void PathNode::add(const std::string &path, const std::vector<RequestHandler> &handlers)
    {
        std::vector<std::string> segments = toSegmentStack(path);
        this->add(segments, handlers);
    }

    void PathNode::add(std::vector<std::string> &path, const std::vector<RequestHandler> &handlers)
    {
        if (path.empty())
        {
            appendHandlers(this->handlers, handlers);
            return;
        }

        std::string key = path.back();
        std::string segment = path.back();
        path.pop_back();

        // A route parameter is stored under ":", the node keeps its name.
        // ":id" => key ":", segment "id"
        if (!key.empty() && key[0] == ':')
        {
            segment = key.substr(1);
            key = ":";
        }

        // operator[] creates the node on first use.
        PathNode &child = nodes[key];
        if (child.segment.empty())
        {
            child.segment = segment;
        }
        child.add(path, handlers);
    }

    std::vector<RequestHandler> PathNode::resolve(const std::string &path, Request &req, bool accumulate)
    {
        std::vector<std::string> segments = toSegmentStack(path);
        return this->resolve(segments, req, accumulate);
    }

    /*
        > When no segment is left
            - Return the handlers of this node.
        > Otherwise (middlewares also collect the handlers of this node on the way)
            - If a literal child matches the segment, descend into it.
            - If a parameter child exists, store the segment as route parameter and descend into it.
            - Otherwise stop with what was collected.
    */
    std::vector<RequestHandler> PathNode::resolve(std::vector<std::string> &path, Request &req, bool accumulate)
    {
        // When no segment is left
        if (path.empty())
        {
            return this->handlers;
        }

        // Otherwise
        std::vector<RequestHandler> handlers;
        if (accumulate)
        {
            appendHandlers(handlers, this->handlers);
        }

        const std::string segment = path.back();
        path.pop_back();

        // Literal child
        auto node = nodes.find(segment);
        if (node != nodes.end())
        {
            appendHandlers(handlers, node->second.resolve(path, req, accumulate));
            return handlers;
        }

        // Parameter child
        node = nodes.find(":");
        if (node != nodes.end())
        {
            req.route.insert({node->second.segment, segment});
            appendHandlers(handlers, node->second.resolve(path, req, accumulate));
            return handlers;
        }

        return handlers;
    }

    // ===========================================================================
    // ============================   RESOLVE PATHS   ============================
    // ===========================================================================
    //
    //   add       Delegates to the root node.
    //   resolve   Delegates to the root node.
    //

    void PathResolver::add(const std::string &path, const std::vector<RequestHandler> &handlers)
    {
        this->root.add(path, handlers);
    }

    std::vector<RequestHandler> PathResolver::resolve(const std::string &path, Request &req, bool accumulate)
    {
        return this->root.resolve(path, req, accumulate);
    }
}
