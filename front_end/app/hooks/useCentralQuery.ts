import { useQuery } from '@tanstack/react-query';
import { UseQueryOptions } from '@tanstack/react-query';


export function useCentralQuery<TData>(
    queryKey: readonly unknown[],
    queryFn: () => Promise<TData>,
    options?: Omit<UseQueryOptions<TData, Error, TData, readonly unknown[]>, "queryKey">
) {
    return useQuery<TData>({
        queryKey,
        queryFn, 
        retry: 1,
        ...options
    });
}