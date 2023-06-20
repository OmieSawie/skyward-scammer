#include "SimpleGameEngine.h"
#include <cmath>
#include <memory>
#include "RPG_Maps.h"
#include "RPG_Assets.h"
#include "RPG_Dynamic.h"

const float PI = 3.14159f;


enum FlipType {
    DOWN, RIGHT, UP, LEFT
};


class Echoes : public GameEngine {
private:
    cMap *pCurrentMap = nullptr;
    RPG_Dynamic *player = nullptr;
    // positions in tiles space
    float fCameraPosX = 0.0f;
    float fCameraPosY = 0.0f;

    int nTileWidth{};
    int nTileHeight{};

public:
    void onUserInputEvent(int eventType, const unsigned char *state, int mouseX,
                          int mouseY, float secPerFrame) override {
        if (state[SDL_SCANCODE_UP]) {
            player->vy -= 10.0f * secPerFrame;
        }
        if (state[SDL_SCANCODE_DOWN]) {
            player->vy += 10.0f * secPerFrame;
        }
        if (state[SDL_SCANCODE_LEFT]) {
            player->vx -= 10.0f * secPerFrame;
        }
        if (state[SDL_SCANCODE_RIGHT]) {
            player->vx += 10.0f * secPerFrame;
        }
    }

    bool onInit() override {
        RPG_Assets::get().loadSprites();
        player = new DynamicCreature("player", RPG_Assets::get().getSprite(0));
        player->px = 10;
        player->py = 10;
        nTileWidth = 24;
        nTileHeight = 24;
        pCurrentMap = new cMap_Village();
        return true;
    }


    bool onFrameUpdate(float fElapsedTime) override {
        // clamp velocities
        if (player->vy > 10) {
            player->vy = 10.0f;
        }
        if (player->vy < -10) {
            player->vy = -10.0f;
        }
        if (player->vx > 10) {
            player->vx = 10.0f;
        }
        if (player->vx < -10) {
            player->vx = -10.0f;
        }

        RPG_Dynamic *object = player;
        float fNewObjectPosX = object->px + object->vx * fElapsedTime;
        float fNewObjectPosY = object->py + object->vy * fElapsedTime;

        // resolve collision along X axis, if any
        if (object->vx < 0) {
            if (pCurrentMap->GetSolid(static_cast<int>(fNewObjectPosX), static_cast<int>(object->py)) ||
                pCurrentMap->GetSolid(static_cast<int>(fNewObjectPosX), static_cast<int>(object->py + 0.9))) {
                // cast the new position to an integer and shift by 1 so that the player is on the boundary
                // of the colliding tile, instead of leaving some space
                fNewObjectPosX = static_cast<int>(fNewObjectPosX) + 1;
                object->vx = 0;
            }
        } else if (object->vx > 0) {
            if (pCurrentMap->GetSolid(static_cast<int>(fNewObjectPosX + 1), static_cast<int>(object->py)) ||
                pCurrentMap->GetSolid(static_cast<int>(fNewObjectPosX + 1), static_cast<int>(object->py + 0.9))) {
                fNewObjectPosX = static_cast<int>(fNewObjectPosX);
                object->vx = 0;
            }
        }
        // check collision along y
        if (object->vy < 0) {
            if (pCurrentMap->GetSolid(static_cast<int>(fNewObjectPosX), static_cast<int>(fNewObjectPosY)) ||
                pCurrentMap->GetSolid(static_cast<int>(fNewObjectPosX + 0.9), static_cast<int>(fNewObjectPosY))) {
                fNewObjectPosY = static_cast<int>(fNewObjectPosY) + 1;
                object->vy = 0;
            }
        } else if (object->vy > 0) {
            if (pCurrentMap->GetSolid(static_cast<int>(fNewObjectPosX), static_cast<int>(fNewObjectPosY + 1)) ||
                pCurrentMap->GetSolid(static_cast<int>(fNewObjectPosX + 0.9), static_cast<int>(fNewObjectPosY + 1))) {
                fNewObjectPosY = static_cast<int>(fNewObjectPosY);
                object->vy = 0;
            }
        }
        object->px = fNewObjectPosX;
        object->py = fNewObjectPosY;

        // apply friction
        object->vx += -4.0f * object->vx * fElapsedTime;
        if (std::abs(object->vx) < 0.01f) {
            object->vx = 0.0f;
        }
        object->vy += -4.0f * object->vy * fElapsedTime;
        if (std::abs(object->vy) < 0.01f) {
            object->vy = 0.0f;
        }
        object->update(fElapsedTime);
        fCameraPosX = player->px;
        fCameraPosY = player->py;
        // Draw Level
        int nVisibleTilesX = mWindowWidth / nTileWidth;
        int nVisibleTilesY = mWindowHeight / nTileHeight;
        // get the top left corner (in tiles space) to be shown on screen
        // camera posX and posY are also in tile space
        float fOffsetX = fCameraPosX - static_cast<float>(nVisibleTilesX) / 2.0f;
        float fOffsetY = fCameraPosY - static_cast<float>(nVisibleTilesY) / 2.0f;
        // clamp
        if (fOffsetX < 0) fOffsetX = 0;
        if (fOffsetY < 0) fOffsetY = 0;
        if (fOffsetX > static_cast<float>(pCurrentMap->nWidth - nVisibleTilesX))
            fOffsetX = static_cast<float>(pCurrentMap->nWidth - nVisibleTilesX);
        if (fOffsetY > static_cast<float>(pCurrentMap->nHeight - nVisibleTilesY))
            fOffsetY = static_cast<float>(pCurrentMap->nHeight - nVisibleTilesY);

        float fTileOffsetX = fOffsetX - static_cast<int>(fOffsetX);
        float fTileOffsetY = fOffsetY - static_cast<int>(fOffsetY);

        // draw each tile
        // we overdraw on the corners to avoid distortion (hacky)
        for (int x = -1; x < nVisibleTilesX + 1; x++) {
            for (int y = -1; y < nVisibleTilesY + 1; y++) {
                int spriteIdx = pCurrentMap->GetIndex(x + static_cast<int>(fOffsetX), y + static_cast<int>(fOffsetY));
                LTexture *texture = RPG_Assets::get().getSprite(spriteIdx);
                texture->drawTexture( static_cast<int>((x - fTileOffsetX) * nTileWidth),
                                     static_cast<int>((y - fTileOffsetY) * nTileHeight), nTileWidth, nTileHeight);
            }
        }
        // draw object
        object->drawSelf(this, fOffsetX, fOffsetY, nTileWidth, nTileHeight);

        LTexture *font = new LTexture();
        font->loadTextureFromText(getFont(), "Hello, everyone!", {0xFF, 0xFF, 0xFF});
        font->drawTexture((mWindowWidth/2 - font->getWidth())/2, mWindowHeight/2 - 20  );
        return true;
    }
};

int main() {
    Echoes echoes;
    echoes.init(30 * 24, 16 * 24, "Echoes Of Deception");
    echoes.startGameLoop();
    return 0;
}
