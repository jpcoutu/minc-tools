/* ----------------------------------------------------------------------------
@COPYRIGHT  :
              Copyright 1993,1994,1995 David MacDonald,
              McConnell Brain Imaging Centre,
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.
---------------------------------------------------------------------------- */

#include  <internal_volume_io.h>

#ifndef lint
static char rcsid[] = "$Header: /private-cvsroot/minc/volume_io/MNI_formats/gen_xf_io.c,v 1.16 1995-07-31 13:44:57 david Exp $";
#endif

/*--------------------- file format keywords ------------------------------ */

static   const char      *TRANSFORM_FILE_HEADER = "MNI Transform File";
static   const char      *TYPE_STRING = "Transform_Type";
static   const char      *LINEAR_TRANSFORM_STRING = "Linear_Transform";
static   const char      *LINEAR_TYPE = "Linear";
static   const char      *THIN_PLATE_SPLINE_STRING =
                                              "Thin_Plate_Spline_Transform";
static   const char      *INVERT_FLAG_STRING = "Invert_Flag";
static   const char      *TRUE_STRING = "True";
static   const char      *FALSE_STRING = "False";
static   const char      *N_DIMENSIONS_STRING = "Number_Dimensions";
static   const char      *POINTS_STRING = "Points";
static   const char      *DISPLACEMENTS_STRING = "Displacements";

static   const char      *GRID_TRANSFORM_STRING = "Grid_Transform";
static   const char      *DISPLACEMENT_VOLUME = "Displacement_Volume";

/* ----------------------------- MNI Header -----------------------------------
@NAME       : get_default_transform_file_suffix
@INPUT      : 
@OUTPUT     : 
@RETURNS    : "xfm"
@DESCRIPTION: Returns the default transform file suffix.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : 1993            David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  char  *get_default_transform_file_suffix()
{
    return( "xfm" );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : output_one_transform
@INPUT      : file
              filename        \ these two used to manufacture unique filenames
              volume_count    / for grid transform volume files.
              invert  - whether to invert the transform
              transform
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Outputs a transform to the MNI transform file.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : 1993            David MacDonald
@MODIFIED   : Feb 21, 1995    David MacDonald : added grid transforms 
---------------------------------------------------------------------------- */

private  void  output_one_transform(
    FILE                *file,
    char                filename[],
    int                 *volume_count,
    BOOLEAN             invert,
    General_transform   *transform )
{
    int        i, c, trans;
    Transform  *lin_transform;
    STRING     volume_filename, base_filename, prefix_filename;

    switch( transform->type )
    {
    case LINEAR:
        (void) fprintf( file, "%s = %s;\n", TYPE_STRING, LINEAR_TYPE );

        (void) fprintf( file, "%s =\n", LINEAR_TRANSFORM_STRING );

        if( invert )
            lin_transform = get_inverse_linear_transform_ptr( transform );
        else
            lin_transform = get_linear_transform_ptr( transform );

        for_less( i, 0, 3 )
        {
            (void) fprintf( file, " %15.8f %15.8f %15.8f %15.8f",
                                  Transform_elem(*lin_transform,i,0),
                                  Transform_elem(*lin_transform,i,1),
                                  Transform_elem(*lin_transform,i,2),
                                  Transform_elem(*lin_transform,i,3) );
            if( i == 2 )
                (void) fprintf( file, ";" );
            (void) fprintf( file, "\n" );
        }
        break;

    case THIN_PLATE_SPLINE:
        (void) fprintf( file, "%s = %s;\n", TYPE_STRING,
                        THIN_PLATE_SPLINE_STRING);

        if( transform->inverse_flag )
            invert = !invert;

        if( invert )
            (void) fprintf( file, "%s = %s;\n", INVERT_FLAG_STRING,TRUE_STRING);

        (void) fprintf( file, "%s = %d;\n", N_DIMENSIONS_STRING,
                        transform->n_dimensions );

        (void) fprintf( file, "%s =\n", POINTS_STRING );

        for_less( i, 0, transform->n_points )
        {
            for_less( c, 0, transform->n_dimensions )
                (void) fprintf( file, " %g", transform->points[i][c] );

            if( i == transform->n_points-1 )
                (void) fprintf( file, ";" );

            (void) fprintf( file, "\n" );
        }

        (void) fprintf( file, "%s =\n", DISPLACEMENTS_STRING );

        for_less( i, 0, transform->n_points + transform->n_dimensions + 1 )
        {
            for_less( c, 0, transform->n_dimensions )
                (void) fprintf( file, " %g", transform->displacements[i][c] );

            if( i == transform->n_points + transform->n_dimensions + 1 - 1 )
                (void) fprintf( file, ";" );

            (void) fprintf( file, "\n" );
        }
        break;

    case GRID_TRANSFORM:
        (void) fprintf( file, "%s = %s;\n", TYPE_STRING,
                        GRID_TRANSFORM_STRING );

        if( transform->inverse_flag )
            invert = !invert;

        if( invert )
            (void) fprintf( file, "%s = %s;\n", INVERT_FLAG_STRING,TRUE_STRING);

        /*--- the volume will be stored in a file of the same prefix as this
              transform, but ending in _grid.mnc */

        if( filename == NULL || strlen(filename) == 0 )
            (void) strcpy( prefix_filename, "grid" );
        else
        {
            (void) strcpy( prefix_filename, filename );
            i = strlen( prefix_filename ) - 1;
            while( i > 0 && prefix_filename[i] != '.' &&
                   prefix_filename[i] != '/' )
                --i;
            if( i >= 0 && prefix_filename[i] == '.' )
                prefix_filename[i] = (char) 0;
        }

        /*--- write out the volume filename to the transform file */

        (void) sprintf( volume_filename, "%s_grid_%d.mnc", prefix_filename,
                        *volume_count );

        /*--- decide where to write the volume file */

        remove_directories_from_filename( volume_filename, base_filename );

        (void) fprintf( file, "%s = %s;\n", DISPLACEMENT_VOLUME,
                        base_filename );

        /*--- write the volume file */

        (void) output_volume( volume_filename, NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                              (Volume) transform->displacement_volume,
                              NULL, NULL );

        break;

    case USER_TRANSFORM:
        print_error( "Cannot output user transformation.\n" );
        output_comments( file, "User transform goes here." );
        break;

    case CONCATENATED_TRANSFORM:
        
        if( transform->inverse_flag )
            invert = !invert;

        if( invert )
        {
            for( trans = get_n_concated_transforms(transform)-1;  trans >= 0;
                 --trans )
            {
                 output_one_transform( file, filename, volume_count, invert,
                               get_nth_general_transform(transform,trans) );
            }
        }
        else
        {
            for_less( trans, 0, get_n_concated_transforms(transform) )
            {
                 output_one_transform( file, filename, volume_count, invert,
                                   get_nth_general_transform(transform,trans) );
            }
        }
        break;
    }
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : output_transform
              file
              filename          \  these two args used to manufacture unique
              volume_count_ptr  /  filenames for grid transform volumes
              comments   - can be null
              transform
@INPUT      : 
@OUTPUT     : 
@RETURNS    : OK or ERROR
@DESCRIPTION: Outputs the transform to the file in MNI transform format.
              The filename is used to define the prefix for writing out
              volumes that are part of grid transforms.  The volume_count_ptr
              is used to give each volume written in a given file a unique
              index, and therefore a unique filename.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : 1993            David MacDonald
@MODIFIED   : Feb. 21, 1995   D. MacDonald
---------------------------------------------------------------------------- */

public  Status  output_transform(
    FILE                *file,
    char                filename[],
    int                 *volume_count_ptr,
    char                comments[],
    General_transform   *transform )
{
    int    volume_count;

    /* --- parameter checking */

    if( file == (FILE *) 0 )
    {
        print_error( "output_transform(): passed NULL FILE ptr.\n");
        return( ERROR );
    }

    /* --- okay write the file */

    (void) fprintf( file, "%s\n", TRANSFORM_FILE_HEADER );

    output_comments( file, comments );
    (void) fprintf( file, "\n" );

    /*--- if the user has not initialized the volume count, then do so */

    if( volume_count_ptr == NULL )
    {
        volume_count = 0;
        volume_count_ptr = &volume_count;
    }

    output_one_transform( file, filename, volume_count_ptr, FALSE, transform );

    return( OK );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : input_one_transform
@INPUT      : file
              filename    - used to get relative paths
@OUTPUT     : transform
@RETURNS    : OK or ERROR
@DESCRIPTION: Inputs a transform from the file.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : 1993            David MacDonald
@MODIFIED   : Feb. 21, 1995   David MacDonald - added grid transforms
---------------------------------------------------------------------------- */

private  Status  input_one_transform(
    FILE                *file,
    char                filename[],
    General_transform   *transform )
{
    Status            status;
    int               i, j, n_points, n_dimensions;
    Real              **points, **displacements;
    Real              value, *points_1d;
    STRING            type_name, str, volume_filename, directory, tmp_filename;
    Volume            volume;
    Transform         linear_transform;
    Transform_types   type;
    BOOLEAN           inverse_flag;
    General_transform inverse;
    minc_input_options  options;

    inverse_flag = FALSE;

    /* --- read the type of transform */

    status = mni_input_keyword_and_equal_sign( file, TYPE_STRING, FALSE );

    if( status != OK )
        return( status );

    if( mni_input_string( file, type_name, MAX_STRING_LENGTH, (char) ';',
                          (char) 0 ) != OK )
    {
        print_error( "input_transform(): missing transform type.\n");
        return( ERROR );
    }
    if( mni_skip_expected_character( file, (char) ';' ) != OK )
        return( ERROR );

    if( strcmp( type_name, LINEAR_TYPE ) == 0 )
        type = LINEAR;
    else if( strcmp( type_name, THIN_PLATE_SPLINE_STRING ) == 0 )
        type = THIN_PLATE_SPLINE;
    else if( strcmp( type_name, GRID_TRANSFORM_STRING ) == 0 )
        type = GRID_TRANSFORM;
    else
    {
        print_error( "input_transform(): invalid transform type.\n");
        return( ERROR );
    }

    /* --- read the next string */

    if( mni_input_string( file, str, MAX_STRING_LENGTH, (char) '=',
                          (char) 0 ) != OK )
        return( ERROR );

    if( strcmp( str, INVERT_FLAG_STRING ) == 0 )
    {
        if( mni_skip_expected_character( file, (char) '=' ) != OK )
            return( ERROR );
        if( mni_input_string( file, str, MAX_STRING_LENGTH, (char) ';',
                              (char) 0 ) != OK )
            return( ERROR );
        if( mni_skip_expected_character( file, (char) ';' ) != OK )
            return( ERROR );

        if( strcmp( str, TRUE_STRING ) == 0 )
            inverse_flag = TRUE;
        else if( strcmp( str, FALSE_STRING ) == 0 )
            inverse_flag = FALSE;
        else
        {
            print_error( "Expected %s or %s after %s =\n",
                         TRUE_STRING, FALSE_STRING,
                   INVERT_FLAG_STRING );
            return( ERROR );
        }

        if( mni_input_string( file, str, MAX_STRING_LENGTH, (char) '=',
                              (char) 0 ) != OK )
            return( ERROR );
    }

    switch( type )
    {
    case LINEAR:
        if( strcmp( str, LINEAR_TRANSFORM_STRING ) != 0 )
        {
            print_error( "Expected %s =\n", LINEAR_TRANSFORM_STRING );
            return( ERROR );
        }

        if( mni_skip_expected_character( file, (char) '=' ) != OK )
            return( ERROR );

        make_identity_transform( &linear_transform );

        /* now read the 3 lines of transforms */

        for_less( i, 0, 3 )
        {
            for_less( j, 0, 4 )
            {
                if( mni_input_real( file, &value ) != OK )
                {
                    print_error(
                    "input_transform(): error reading transform elem [%d,%d]\n",
                    i+1, j+1 );
                    return( ERROR );
                }

                Transform_elem(linear_transform,i,j) = value;
            }
        }

        if( mni_skip_expected_character( file, (char) ';' ) != OK )
            return( ERROR );

        create_linear_transform( transform, &linear_transform );

        break;

    case THIN_PLATE_SPLINE:

        /* --- read Number_Dimensions = 3; */

        if( strcmp( str, N_DIMENSIONS_STRING ) != 0 )
        {
            print_error( "Expected %s =\n", N_DIMENSIONS_STRING );
            return( ERROR );
        }
        if( mni_skip_expected_character( file, (char) '=' ) != OK )
            return( ERROR );
        if( mni_input_int( file, &n_dimensions ) != OK )
            return( ERROR );
        if( mni_skip_expected_character( file, (char) ';' ) != OK )
            return( ERROR );

        /* --- read Points = x y z x y z .... ; */

        if( mni_input_keyword_and_equal_sign( file, POINTS_STRING, TRUE ) != OK)
            return( ERROR );
        if( mni_input_reals( file, &n_points, &points_1d ) != OK )
            return( ERROR );

        if( n_points % n_dimensions != 0 )
        {
            print_error(
        "Number of points (%d) must be multiple of number of dimensions (%d)\n",
                  n_points, n_dimensions );
            return( ERROR );
        }

        n_points = n_points / n_dimensions;

        ALLOC2D( points, n_points, n_dimensions );
        for_less( i, 0, n_points )
        {
            for_less( j, 0, n_dimensions )
            {
                points[i][j] = points_1d[IJ(i,j,n_dimensions)];
            }
        }

        /* --- allocate and input the displacements */

        ALLOC2D( displacements, n_points + n_dimensions + 1, n_dimensions );

        if( mni_input_keyword_and_equal_sign( file, DISPLACEMENTS_STRING, TRUE )
                                                                       != OK )
            return( ERROR );

        for_less( i, 0, n_points + n_dimensions + 1 )
        {
            for_less( j, 0, n_dimensions )
            {
                if( mni_input_real( file, &value ) != OK )
                {
                    print_error( "Expected more displacements.\n" );
                    return( ERROR );
                }
                displacements[i][j] = value;
            }
        }

        if( mni_skip_expected_character( file, (char) ';' ) != OK )
            return( ERROR );

        create_thin_plate_transform_real( transform, n_dimensions,
                                          n_points, points, displacements );
        break;

    case GRID_TRANSFORM:

        /*--- read the displacement volume filename */

        if( strcmp( str, DISPLACEMENT_VOLUME ) != 0 )
        {
            print_error( "Expected %s =\n", DISPLACEMENT_VOLUME );
            return( ERROR );
        }
        if( mni_skip_expected_character( file, (char) '=' ) != OK )
            return( ERROR );

        if( mni_input_string( file, volume_filename, MAX_STRING_LENGTH,
                              (char) ';', (char) 0 ) != OK )
            return( ERROR );

        if( mni_skip_expected_character( file, (char) ';' ) != OK )
            return( ERROR );

        /*--- if the volume filename is relative, add the required directory */

        if( volume_filename[0] != '/' && filename != NULL )
        {
            extract_directory( filename, directory );

            if( strlen(directory) != 0 )
            {
                (void) strcpy( tmp_filename, volume_filename );
                (void) sprintf( volume_filename, "%s/%s", directory,
                                tmp_filename );
            }
        }

        /*--- input the displacement volume */

        set_default_minc_input_options( &options );
        set_minc_input_vector_to_scalar_flag( &options, FALSE );

        if( input_volume( volume_filename, 4, NULL, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, TRUE, &volume, &options ) != OK )
            return( ERROR );

        /*--- create the transform */

        create_grid_transform_no_copy( transform, volume );

        break;
    }

    if( inverse_flag )
    {
        create_inverse_general_transform( transform, &inverse );
        delete_general_transform( transform );
        *transform = inverse;
    }

    return( OK );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : input_transform
@INPUT      : file
              filename    - used to define directory for relative filename
@OUTPUT     : transform
@RETURNS    : OK or ERROR
@DESCRIPTION: Inputs the transform from the file.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : 1993            David MacDonald
@MODIFIED   : Feb. 21, 1995   D. MacDonald
---------------------------------------------------------------------------- */

public  Status  input_transform(
    FILE                *file,
    char                filename[],
    General_transform   *transform )
{
    Status              status;
    int                 n_transforms;
    STRING              line;
    General_transform   next, concated;

    /* parameter checking */

    if( file == (FILE *) 0 )
    {
        print_error( "input_transform(): passed NULL FILE ptr.\n");
        return( ERROR );
    }

    /* okay read the header */

    if( mni_input_string( file, line, MAX_STRING_LENGTH, (char) 0, (char) 0 )
            != OK ||
        strcmp( line, TRANSFORM_FILE_HEADER ) != 0 )
    {
        print_error( "input_transform(): invalid header in file.\n");
        return( ERROR );
    }

    n_transforms = 0;
    while( (status = input_one_transform( file, filename, &next )) == OK )
    {
        if( n_transforms == 0 )
            *transform = next;
        else
        {
            concat_general_transforms( transform, &next, &concated );
            delete_general_transform( transform );
            delete_general_transform( &next );
            *transform = concated;
        }
        ++n_transforms;
    }

    if( status == ERROR )
    {
        print_error( "input_transform: error reading transform.\n" );
        return( ERROR );
    }
    else if( n_transforms == 0 )
    {
        print_error( "input_transform: no transform present.\n" );
        return( ERROR );
    }

    return( OK );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : output_transform_file
@INPUT      : filename
              comments
              transform
@OUTPUT     : 
@RETURNS    : OK or ERROR
@DESCRIPTION: Opens the file, outputs the transform, and closes the file.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : 1993            David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  Status  output_transform_file(
    char                filename[],
    char                comments[],
    General_transform   *transform )
{
    Status  status;
    FILE    *file;
    int     volume_count;

    status = open_file_with_default_suffix( filename,
                      get_default_transform_file_suffix(),
                      WRITE_FILE, ASCII_FORMAT, &file );

    volume_count = 0;

    if( status == OK )
        status = output_transform( file, filename, &volume_count,
                                   comments, transform );

    if( status == OK )
        status = close_file( file );

    return( status );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : input_transform_file
@INPUT      : filename
@OUTPUT     : transform
@RETURNS    : OK or ERROR
@DESCRIPTION: Opens the file, inputs the transform, and closes the file.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : 1993            David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  Status  input_transform_file(
    char                filename[],
    General_transform   *transform )
{
    Status  status;
    FILE    *file;

    status = open_file_with_default_suffix( filename,
                      get_default_transform_file_suffix(),
                      READ_FILE, ASCII_FORMAT, &file );

    if( status == OK )
        status = input_transform( file, filename, transform );

    if( status == OK )
        status = close_file( file );

    return( status );
}
