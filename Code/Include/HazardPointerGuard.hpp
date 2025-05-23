// Copyright (c) 2025 Kayou Corporation. All Rights Reserved.

#pragma once

struct HazardPointer;

class HazardPointerGuard 
{
public:
    HazardPointerGuard();
    ~HazardPointerGuard();

    [[nodiscard]] HazardPointer* Get() const { return m_hp; }

private:
    HazardPointer* m_hp;
};
