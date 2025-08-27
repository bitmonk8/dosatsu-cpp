//===--- CommentProcessor.cpp - Comment and documentation processing ----===//
//
// Part of the Dosatsu project
//
//===----------------------------------------------------------------------===//

#include "CommentProcessor.h"

#include "ASTNodeProcessor.h"
#include "KuzuDatabase.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/CommentVisitor.h"
#include "clang/AST/DeclCXX.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <algorithm>

using namespace clang;
using namespace clang::comments;

CommentProcessor::CommentProcessor(KuzuDatabase& database, ASTNodeProcessor& nodeProcessor, ASTContext& astContext)
    : database(database), astContext(&astContext)
{
    (void)nodeProcessor;  // Unused parameter for now - kept for future expansion
}

void CommentProcessor::processComments(const clang::Decl* decl, int64_t declId)
{
    if (!database.isInitialized() || (decl == nullptr) || declId == -1)
        return;

    const RawComment* rawComment = astContext->getRawCommentForDeclNoCache(decl);
    if (rawComment == nullptr)
        return;

    try
    {
        // Get the comment text
        std::string commentText = rawComment->getRawText(astContext->getSourceManager()).str();

        // Determine if this is a documentation comment
        bool isDocumentation = rawComment->isDocumentation();

        std::string commentKind = isDocumentation ? "documentation" : "regular";
        std::string briefText;
        std::string detailedText;

        // Try to parse structured comment if it's documentation
        if (isDocumentation)
        {
            const FullComment* fullComment = astContext->getCommentForDecl(decl, nullptr);
            if (fullComment != nullptr)
            {
                // Extract brief and detailed text from structured comment
                briefText = extractCommentText(fullComment);
                detailedText = briefText;  // For now, use same text for both
            }
        }

        // Create a comment node (generate unique ID since RawComment is not an AST node)
        static int64_t commentIdCounter = 1000000;  // Start high to avoid conflicts
        int64_t commentNodeId = commentIdCounter++;

        createCommentNode(commentNodeId, commentText, commentKind, isDocumentation, briefText, detailedText);
        createCommentRelation(declId, commentNodeId);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception processing comments: " << e.what() << "\n";
    }
}

void CommentProcessor::createCommentNode(int64_t nodeId,
                                         const std::string& commentText,
                                         const std::string& commentKind,
                                         bool isDocumentationComment,
                                         const std::string& briefText,
                                         const std::string& detailedText)
{
    if (!database.isInitialized())
        return;

    try
    {
        // Escape single quotes and clean up text for database storage
        std::string cleanCommentText = KuzuDatabase::escapeString(commentText);
        std::string cleanBriefText = KuzuDatabase::escapeString(briefText);
        std::string cleanDetailedText = KuzuDatabase::escapeString(detailedText);
        std::string escapedCommentKind = KuzuDatabase::escapeString(commentKind);

        // Limit text length for database storage
        if (cleanCommentText.length() > 1000)
            cleanCommentText = cleanCommentText.substr(0, 1000) + "...";
        if (cleanBriefText.length() > 500)
            cleanBriefText = cleanBriefText.substr(0, 500) + "...";
        if (cleanDetailedText.length() > 2000)
            cleanDetailedText = cleanDetailedText.substr(0, 2000) + "...";

        std::string query = "CREATE (c:Comment {node_id: " + std::to_string(nodeId) + ", comment_text: '" +
                            cleanCommentText + "', comment_kind: '" + escapedCommentKind +
                            "', is_documentation: " + (isDocumentationComment ? "true" : "false") + ", brief_text: '" +
                            cleanBriefText + "', detailed_text: '" + cleanDetailedText + "'})";

        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating Comment node: " << e.what() << "\n";
    }
}

void CommentProcessor::createCommentRelation(int64_t declId, int64_t commentId)
{
    if (!database.isInitialized() || declId == -1 || commentId == -1)
        return;

    try
    {
        std::map<std::string, std::string> properties;
        properties["attachment_type"] = "documentation";
        database.addRelationshipToBatch(declId, commentId, "HAS_COMMENT", properties);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating HAS_COMMENT relationship: " << e.what() << "\n";
    }
}

auto CommentProcessor::extractCommentKind(const clang::comments::Comment* comment) -> std::string
{
    if (comment == nullptr)
        return "unknown";

    // Simplified comment kind extraction - the specific enum values
    // may vary between LLVM versions
    return comment->getCommentKindName();
}

auto CommentProcessor::extractCommentText(const clang::comments::Comment* comment) -> std::string
{
    if (comment == nullptr)
        return "";

    // This is a simplified text extraction
    // A full implementation would traverse the comment AST
    return "comment_text_placeholder";
}

auto CommentProcessor::isDocumentationComment(const clang::comments::Comment* comment) -> bool
{
    // Documentation comments typically have specific structure
    return comment != nullptr && isa<FullComment>(comment);
}
