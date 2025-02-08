import { useCallback } from 'react';
import { useState } from 'react';


export function useApi<T>(url: string, options?: RequestInit) {
    const [data, setData] = useState<T | null>(null);
    const [error, setError] = useState<string | null>(null);
    const [loading, setLoading] = useState<boolean>(false);

    const fetchData = useCallback(async () => {
        setLoading(true);
        setError(null);
        try {
            const response = await fetch(url, options);
            const json = await response.json();
            if (!response.ok) {
                throw new Error(json.message || `Error ${response.status}`);
            }
            setData(json);
        } catch (err: unknown) {
            if (err instanceof Error) {
                setError(err.message);
                console.error(`Error fetching ${url}:`, err.message);
            } else {
                setError(String(err));
                console.error(`Error fetching ${url}:`, err);
            }
        } finally {
            setLoading(false);
        }
    }, [url, options]);

    return { data, error, loading, refetch: fetchData };
}