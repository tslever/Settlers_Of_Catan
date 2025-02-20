type QueryBoundaryProps = {
    isLoading: boolean;
    error: unknown;
    children: React.ReactNode;
};


const QueryBoundary: React.FC<QueryBoundaryProps> = ({ isLoading, error, children }) => {
    if (isLoading) {
        return <div className = "loading">Loading...</div>;
    }
    if (error) {
        return <div className = "error">Error: {(error as Error).message}</div>;
    }
    return <>{children}</>
}


export default QueryBoundary;