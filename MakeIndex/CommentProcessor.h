//===--- CommentProcessor.h - Comment and documentation processing ------===//
//
// Part of the MakeIndex project
//
//===----------------------------------------------------------------------===//

#pragma once

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RawCommentList.h"
#include "clang/AST/Comment.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <cstdint>
#include <string>

namespace clang
{

class KuzuDatabase;
class ASTNodeProcessor;

/// Handles comment and documentation processing for AST storage
class CommentProcessor
{
public:
    /// Constructor
    /// \param database Database instance for storage
    /// \param nodeProcessor Node processor for creating basic nodes
    /// \param astContext AST context for comment analysis
    CommentProcessor(KuzuDatabase& database, ASTNodeProcessor& nodeProcessor, ASTContext& astContext);

    /// Process comments for a declaration
    /// \param decl Declaration to process comments for
    /// \param declId Node ID of the declaration
    void processComments(const clang::Decl* decl, int64_t declId);

    /// Create comment node
    /// \param nodeId Node ID for the comment
    /// \param commentText Text content of the comment
    /// \param commentKind Kind of comment
    /// \param isDocumentationComment Whether this is documentation
    /// \param briefText Brief description (for doc comments)
    /// \param detailedText Detailed description (for doc comments)
    void createCommentNode(int64_t nodeId,
                           const std::string& commentText,
                           const std::string& commentKind,
                           bool isDocumentationComment,
                           const std::string& briefText = "",
                           const std::string& detailedText = "");

    /// Create comment relationship
    /// \param declId Declaration node ID
    /// \param commentId Comment node ID
    void createCommentRelation(int64_t declId, int64_t commentId);

    /// Extract comment kind
    /// \param comment Comment AST node
    /// \return String representation of comment kind
    auto extractCommentKind(const clang::comments::Comment* comment) -> std::string;

    /// Extract comment text
    /// \param comment Comment AST node
    /// \return Text content of the comment
    auto extractCommentText(const clang::comments::Comment* comment) -> std::string;

    /// Check if comment is documentation
    /// \param comment Comment AST node
    /// \return True if this is a documentation comment
    auto isDocumentationComment(const clang::comments::Comment* comment) -> bool;

private:
    KuzuDatabase& database;
    const ASTContext* astContext;
};

}  // namespace clang
