#!/bin/bash
# Manual deployment script for GitHub Pages
# This script builds the web version and deploys to gh-pages branch

set -e

echo "========================================="
echo "Poor House Juno - Manual GitHub Pages Deploy"
echo "========================================="
echo ""

# Check if git is clean
if [[ -n $(git status -s) ]]; then
    echo "Warning: You have uncommitted changes."
    echo "These will not be included in the deployment."
    read -p "Continue anyway? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Store current branch
CURRENT_BRANCH=$(git branch --show-current)
echo "Current branch: $CURRENT_BRANCH"
echo ""

# Step 1: Build the web version
echo "Step 1/4: Building web version..."
./scripts/build_web.sh

# Step 2: Verify build outputs exist
if [[ ! -f "web/synth-processor.js" ]] || [[ ! -f "web/synth-processor.wasm" ]]; then
    echo "Error: Build failed - output files not found in web/ directory"
    exit 1
fi

# Step 3: Deploy to gh-pages branch
echo ""
echo "Step 2/4: Preparing gh-pages branch..."

# Create or checkout gh-pages branch
if git show-ref --verify --quiet refs/heads/gh-pages; then
    git checkout gh-pages
else
    echo "Creating new gh-pages branch..."
    git checkout --orphan gh-pages
    git rm -rf . 2>/dev/null || true
fi

# Step 4: Copy web files
echo ""
echo "Step 3/4: Copying web files..."
git checkout "$CURRENT_BRANCH" -- web/
mv web/* .
rm -rf web/

# Create .nojekyll to prevent Jekyll processing
touch .nojekyll

# Step 5: Commit and push
echo ""
echo "Step 4/4: Committing and pushing..."
git add -A
if git diff --staged --quiet; then
    echo "No changes to deploy."
else
    COMMIT_MSG="Deploy from $CURRENT_BRANCH ($(git rev-parse --short $CURRENT_BRANCH))"
    git commit -m "$COMMIT_MSG"

    echo ""
    echo "Pushing to origin/gh-pages..."
    git push -u origin gh-pages

    echo ""
    echo "========================================="
    echo "Deployment complete!"
    echo "========================================="
    echo ""
    echo "Your site will be available at:"
    echo "  https://$(git config --get remote.origin.url | sed 's/.*github.com[:/]\(.*\)\.git/\1/' | sed 's/.*github.com[:/]\(.*\)/\1/').github.io/$(basename $(git rev-parse --show-toplevel))/"
    echo ""
    echo "Note: It may take a few minutes for GitHub to build and deploy your site."
    echo "Configure GitHub Pages in repository Settings > Pages > Source: gh-pages branch"
fi

# Return to original branch
echo ""
echo "Returning to $CURRENT_BRANCH branch..."
git checkout "$CURRENT_BRANCH"

echo ""
echo "Done!"
