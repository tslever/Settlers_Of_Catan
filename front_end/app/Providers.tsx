'use client';


import React from 'react';
import { QueryClient } from '@tanstack/react-query';
import { QueryClientProvider } from '@tanstack/react-query';
import { useState } from 'react';


export default function Providers({ children }: { children: React.ReactNode }) {
    const [queryClient] = useState(() => new QueryClient());
    return (
        <QueryClientProvider client = {queryClient}>
            {children}
        </QueryClientProvider>
    );
}